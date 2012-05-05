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


static int work_cb_running     = 0;
static int work_cb_running_max = 0;
static uv_mutex_t work_cb_mutex;


static void work_cb(uv_work_t* work) {
  uv_mutex_lock(&work_cb_mutex);

  work_cb_running++;

  if (work_cb_running > work_cb_running_max) {
    work_cb_running_max = work_cb_running;
  }

  uv_mutex_unlock(&work_cb_mutex);


  /* We don't want the mutex lock here or else work_cb_running
   * would only reach 1 even with multiple threads running.
   */


  /* 100 miliseconds should be long enough for both threads to reach this point */
  uv_sleep(100);


  uv_mutex_lock(&work_cb_mutex);
  work_cb_running--;
  uv_mutex_unlock(&work_cb_mutex);
}


static void after_work_cb(uv_work_t* work) {
  free(work);
}


void add_work(void) {
  uv_work_t* w = malloc(sizeof(*w));
  ASSERT(w != NULL);

  uv_queue_work(uv_default_loop(), w, work_cb, after_work_cb);
}


TEST_IMPL(threadpool_parallel) {
  int i;

  uv_mutex_init(&work_cb_mutex);

  uv_set_parallel(1);

  add_work();
  add_work();

  uv_run(uv_default_loop());

  ASSERT(work_cb_running == 0);
  ASSERT(work_cb_running_max == 1);

  return 0;
}
