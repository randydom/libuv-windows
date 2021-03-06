/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "uv.h"
#include "task.h"


#include <string.h>
#include <errno.h>


TEST_IMPL(tty) {
  int r, width, height;
  int ttyin_fd, ttyout_fd;
  uv_tty_t tty_in, tty_out;
  uv_loop_t* loop = uv_default_loop();

  ASSERT(ttyin_fd >= 0);
  ASSERT(ttyout_fd >= 0);

  ASSERT(UV_UNKNOWN_HANDLE == uv_guess_handle(-1));

  ASSERT(UV_TTY == uv_guess_handle(ttyin_fd));
  ASSERT(UV_TTY == uv_guess_handle(ttyout_fd));

  r = uv_tty_init(uv_default_loop(), &tty_in, ttyin_fd, 1);  /* Readable. */
  ASSERT(r == 0);

  r = uv_tty_init(uv_default_loop(), &tty_out, ttyout_fd, 0);  /* Writable. */
  ASSERT(r == 0);

  r = uv_tty_get_winsize(&tty_out, &width, &height);
  ASSERT(r == 0);

  printf("width=%d height=%d\n", width, height);

  if (width == 0 && height == 0) {
   /* Some environments such as containers or Jenkins behave like this
    * sometimes */
    MAKE_VALGRIND_HAPPY();
    return TEST_SKIP;
  }

  /*
   * Is it a safe assumption that most people have terminals larger than
   * 10x10?
   */
  ASSERT(width > 10);
  ASSERT(height > 10);

  /* Turn on raw mode. */
  r = uv_tty_set_mode(&tty_in, UV_TTY_MODE_RAW);
  ASSERT(r == 0);

  /* Turn off raw mode. */
  r = uv_tty_set_mode(&tty_in, UV_TTY_MODE_NORMAL);
  ASSERT(r == 0);

  /* Calling uv_tty_reset_mode() repeatedly should not clobber errno. */
  errno = 0;
  ASSERT(0 == uv_tty_reset_mode());
  ASSERT(0 == uv_tty_reset_mode());
  ASSERT(0 == uv_tty_reset_mode());
  ASSERT(0 == errno);

  /* TODO check the actual mode! */

  uv_close((uv_handle_t*) &tty_in, NULL);
  uv_close((uv_handle_t*) &tty_out, NULL);

  uv_run(loop, UV_RUN_DEFAULT);

  MAKE_VALGRIND_HAPPY();
  return 0;
}

TEST_IMPL(tty_file)
{
  uv_loop_t loop;
  uv_tty_t tty;
  int fd;

  ASSERT(0 == uv_loop_init(&loop));

  fd = open("test/fixtures/empty_file", O_RDONLY);
  if (fd != -1) {
    ASSERT(UV_EINVAL == uv_tty_init(&loop, &tty, fd, 1));
    ASSERT(0 == close(fd));
  }

/* Bug on AIX where '/dev/random' returns 1 from isatty() */
  fd = open("/dev/random", O_RDONLY);
  if (fd != -1) {
    ASSERT(UV_EINVAL == uv_tty_init(&loop, &tty, fd, 1));
    ASSERT(0 == close(fd));
  }

  fd = open("/dev/zero", O_RDONLY);
  if (fd != -1) {
    ASSERT(UV_EINVAL == uv_tty_init(&loop, &tty, fd, 1));
    ASSERT(0 == close(fd));
  }

  fd = open("/dev/tty", O_RDONLY);
  if (fd != -1) {
    ASSERT(0 == uv_tty_init(&loop, &tty, fd, 1));
    ASSERT(0 == close(fd));
    uv_close((uv_handle_t*) &tty, NULL);
  }

  ASSERT(0 == uv_run(&loop, UV_RUN_DEFAULT));
  ASSERT(0 == uv_loop_close(&loop));

  MAKE_VALGRIND_HAPPY();
  return 0;
}

TEST_IMPL(tty_pty) {
  int master_fd, slave_fd, r;
  struct winsize w;
  uv_loop_t loop;
  uv_tty_t master_tty, slave_tty;

  ASSERT(0 == uv_loop_init(&loop));

  r = openpty(&master_fd, &slave_fd, NULL, NULL, &w);
  if (r != 0)
    RETURN_SKIP("No pty available, skipping.");

  ASSERT(0 == uv_tty_init(&loop, &slave_tty, slave_fd, 0));
  ASSERT(0 == uv_tty_init(&loop, &master_tty, master_fd, 0));
  /* Check if the file descriptor was reopened. If it is,
   * UV_STREAM_BLOCKING (value 0x80) isn't set on flags.
   */
  ASSERT(0 == (slave_tty.flags & 0x80));
  /* The master_fd of a pty should never be reopened.
   */
  ASSERT(master_tty.flags & 0x80);
  ASSERT(0 == close(slave_fd));
  uv_close((uv_handle_t*) &slave_tty, NULL);
  ASSERT(0 == close(master_fd));
  uv_close((uv_handle_t*) &master_tty, NULL);

  ASSERT(0 == uv_run(&loop, UV_RUN_DEFAULT));

  MAKE_VALGRIND_HAPPY();
  return 0;
}
