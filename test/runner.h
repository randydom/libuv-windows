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

#ifndef RUNNER_H_
#define RUNNER_H_

#include <limits.h> /* PATH_MAX */
#include <stdio.h> /* FILE */


/*
 * The maximum number of processes (main + helpers) that a test / benchmark
 * can have.
 */
#define MAX_PROCESSES 8


/*
 * Struct to store both tests and to define helper processes for tasks.
 */
typedef struct {
  char *task_name;
  char *process_name;
  int (*main)(void);
  int is_helper;
  int show_output;

  /*
   * The time in milliseconds after which a single test or benchmark times out.
   */
  int timeout;
} task_entry_t, bench_entry_t;


/*
 * Macros used by test-list.h and benchmark-list.h.
 */
#define TASK_LIST_START                             \
  task_entry_t TASKS[] = {

#define TASK_LIST_END                               \
    { 0, 0, 0, 0, 0, 0 }                               \
  };

#define TEST_DECLARE(name)                          \
  int run_test_##name(void);

#define TEST_ENTRY(name)                            \
    { #name, #name, &run_test_##name, 0, 0, 5000 },

#define TEST_ENTRY_CUSTOM(name, is_helper, show_output, timeout) \
    { #name, #name, &run_test_##name, is_helper, show_output, timeout },

#define BENCHMARK_DECLARE(name)                     \
  int run_benchmark_##name(void);

#define BENCHMARK_ENTRY(name)                       \
    { #name, #name, &run_benchmark_##name, 0, 0, 60000 },

#define HELPER_DECLARE(name)                        \
  int run_helper_##name(void);

#define HELPER_ENTRY(task_name, name)               \
    { #task_name, #name, &run_helper_##name, 1, 0, 0 },

#define TEST_HELPER       HELPER_ENTRY
#define BENCHMARK_HELPER  HELPER_ENTRY
extern char executable_path[PATH_MAX];

/*
 * Include platform-dependent definitions
 */
# include "runner-unix.h"


/* The array that is filled by test-list.h or benchmark-list.h */
extern task_entry_t TASKS[];

/*
 * Run all tests.
 */
int run_tests(int benchmark_output);

/* Run a single test. Starts up any helpers. */
int run_test(const char* test, int benchmark_output,int test_count);

/*
 * Run a test part, i.e. the test or one of its helpers.
 */
int run_test_part(const char* test, const char* part);


/*
 * Print tests in sorted order to `stream`. Used by `./run-tests --list`.
 */
void print_tests(FILE* stream);

/* Print lines in |buffer| as TAP diagnostics to |stream|. */
void print_lines(const char* buffer, size_t size, FILE* stream);

/*
 * Stuff that should be implemented by test-runner-<platform>.h
 * All functions return 0 on success, -1 on failure, unless specified
 * otherwise.
 */

/* Do platform-specific initialization. */
int platform_init(int argc, char** argv);

/* Invoke "argv[0] test-name [test-part]". Store process info in *p. */
/* Make sure that all stdio output of the processes is buffered up. */
int thread_start(char *name, char* part, int (*test_routine)(void) ,  thread_info_t *p, int is_helper);

/* Wait for all `n` processes in `vec` to terminate. */
/* Time out after `timeout` msec, or never if timeout == -1 */
/* Return 0 if all processes are terminated, -1 on error, -2 on timeout. */
int thread_wait(thread_info_t *vec, int n, int timeout);

/* Returns the number of bytes in the stdio output buffer for process `p`. */
long int thread_output_size(thread_info_t *p);

/* Copy the contents of the stdio output buffer to `stream`. */
int thread_copy_output(thread_info_t* p, FILE* stream);

/* Copy the last line of the stdio output buffer to `buffer` */
int thread_read_last_line(thread_info_t *p,char * buffer,size_t buffer_len);

/* Return the name that was specified when `p` was started by process_start */
char* thread_get_name(thread_info_t *p);

/* Terminate process `p`. */
int thread_terminate(thread_info_t *p);

/* Return the exit code of process p. On error, return -1. */
int thread_reap(thread_info_t *p);

/* Clean up after terminating process `p` (e.g. free the output buffer etc.). */
void thread_cleanup(thread_info_t *p);

/* Move the console cursor one line up and back to the first column. */
void rewind_cursor(void);

#endif /* RUNNER_H_ */
