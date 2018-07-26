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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>
# include <unistd.h>  /* close */

static uv_pipe_t pipe_client;
static uv_pipe_t pipe_server;
static uv_connect_t connect_req;

static int pipe_close_cb_called = 0;
static int pipe_client_connect_cb_called = 0;


static void pipe_close_cb(uv_handle_t* handle) {
  ASSERT(handle == (uv_handle_t*) &pipe_client ||
         handle == (uv_handle_t*) &pipe_server);
  pipe_close_cb_called++;
}


static void pipe_client_connect_cb(uv_connect_t* req, int status) {
  char buf[1024];
  size_t len;
  int r;

  ASSERT(req == &connect_req);
  ASSERT(status == 0);

  len = sizeof buf;
  r = uv_pipe_getpeername(&pipe_client, buf, &len);
  ASSERT(r == 0);

  ASSERT(buf[len - 1] != 0);
  ASSERT(memcmp(buf, TEST_PIPENAME, len) == 0);

  len = sizeof buf;
  r = uv_pipe_getsockname(&pipe_client, buf, &len);
  ASSERT(r == 0 && len == 0);

  pipe_client_connect_cb_called++;


  uv_close((uv_handle_t*) &pipe_client, pipe_close_cb);
  uv_close((uv_handle_t*) &pipe_server, pipe_close_cb);
}


static void pipe_server_connection_cb(uv_stream_t* handle, int status) {
  /* This function *may* be called, depending on whether accept or the
   * connection callback is called first.
   */
  ASSERT(status == 0);
}


TEST_IMPL(pipe_getsockname) {
  uv_loop_t* loop;
  char buf[1024];
  size_t len;
  int r;

  loop = uv_default_loop();
  ASSERT(loop != NULL);

  r = uv_pipe_init(loop, &pipe_server, 0);
  ASSERT(r == 0);

  len = sizeof buf;
  r = uv_pipe_getsockname(&pipe_server, buf, &len);
  ASSERT(r == UV_EBADF);

  len = sizeof buf;
  r = uv_pipe_getpeername(&pipe_server, buf, &len);
  ASSERT(r == UV_EBADF);

  r = uv_pipe_bind(&pipe_server, TEST_PIPENAME);
  ASSERT(r == 0);

  len = sizeof buf;
  r = uv_pipe_getsockname(&pipe_server, buf, &len);
  ASSERT(r == 0);

  ASSERT(buf[len - 1] != 0);
  ASSERT(buf[len] == '\0');
  ASSERT(memcmp(buf, TEST_PIPENAME, len) == 0);

  len = sizeof buf;
  r = uv_pipe_getpeername(&pipe_server, buf, &len);
  ASSERT(r == UV_ENOTCONN);

  r = uv_listen((uv_stream_t*) &pipe_server, 0, pipe_server_connection_cb);
  ASSERT(r == 0);

  r = uv_pipe_init(loop, &pipe_client, 0);
  ASSERT(r == 0);

  len = sizeof buf;
  r = uv_pipe_getsockname(&pipe_client, buf, &len);
  ASSERT(r == UV_EBADF);

  len = sizeof buf;
  r = uv_pipe_getpeername(&pipe_client, buf, &len);
  ASSERT(r == UV_EBADF);

  uv_pipe_connect(&connect_req, &pipe_client, TEST_PIPENAME, pipe_client_connect_cb);

  len = sizeof buf;
  r = uv_pipe_getsockname(&pipe_client, buf, &len);
  ASSERT(r == 0 && len == 0);

  len = sizeof buf;
  r = uv_pipe_getpeername(&pipe_client, buf, &len);
  ASSERT(r == 0);

  ASSERT(buf[len - 1] != 0);
  ASSERT(memcmp(buf, TEST_PIPENAME, len) == 0);

  r = uv_run(loop, UV_RUN_DEFAULT);
  ASSERT(r == 0);
  ASSERT(pipe_client_connect_cb_called == 1);
  ASSERT(pipe_close_cb_called == 2);

  MAKE_VALGRIND_HAPPY();
  return 0;
}


TEST_IMPL(pipe_getsockname_abstract) {
#if defined(__linux__)
  char buf[1024];
  size_t len;
  int r;
  int sock;
  struct sockaddr_un sun;
  socklen_t sun_len;
  char abstract_pipe[] = "\0test-pipe";

  sock = socket(AF_LOCAL, SOCK_STREAM, 0);
  ASSERT(sock != -1);

  sun_len = sizeof sun;
  memset(&sun, 0, sun_len);
  sun.sun_family = AF_UNIX;
  memcpy(sun.sun_path, abstract_pipe, sizeof abstract_pipe);

  r = bind(sock, (struct sockaddr*)&sun, sun_len);
  ASSERT(r == 0);

  r = uv_pipe_init(uv_default_loop(), &pipe_server, 0);
  ASSERT(r == 0);
  r = uv_pipe_open(&pipe_server, sock);
  ASSERT(r == 0);

  len = sizeof buf;
  r = uv_pipe_getsockname(&pipe_server, buf, &len);
  ASSERT(r == 0);

  ASSERT(memcmp(buf, abstract_pipe, sizeof abstract_pipe) == 0);

  uv_close((uv_handle_t*)&pipe_server, pipe_close_cb);

  uv_run(uv_default_loop(), UV_RUN_DEFAULT);

  close(sock);

  ASSERT(pipe_close_cb_called == 1);
  MAKE_VALGRIND_HAPPY();
  return 0;
#else
  MAKE_VALGRIND_HAPPY();
  return 0;
#endif
}

TEST_IMPL(pipe_getsockname_blocking) {
  MAKE_VALGRIND_HAPPY();
  return 0;
}