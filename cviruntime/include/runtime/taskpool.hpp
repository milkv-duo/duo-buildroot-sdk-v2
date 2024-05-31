/*
 * This file is licensed under the zlib/libpng license, included in this
 * distribution in the file COPYING.
 */
#ifndef RUNTIME_TASKQUE_H
#define RUNTIME_TASKQUE_H

#include <future>
#include <thread>
#include <deque>
#include <vector>
#include <utility>
#include <chrono>
#include <list>
#include <mutex>
#include <condition_variable>
#include "cviruntime.h"

namespace cvi {
namespace runtime {

class TaskPool;

class Task {
public:
  Task(TaskPool *pool, void *program, CVI_TENSOR *inputs, int input_num,
       CVI_TENSOR *outputs, int output_num);

  void *program;
  int input_num;
  int output_num;
  CVI_TENSOR *inputs;
  CVI_TENSOR *outputs;
  CVI_RC retCode = CVI_RC_UNINIT;
};

class RingQueue {
public:
  RingQueue(int capacity) : _capacity(capacity) { _queue.resize(_capacity); }

  ~RingQueue() {}

  void put(Task *task) {
    std::unique_lock<std::mutex> lock(_mutex);
    while (_capacity - _length <= 1) {
      _cond_idel.wait(lock);
    }
    _queue[_tail] = task;
    move(_tail);
    _length++;
    _cond_busy.notify_one();
  }

  Task *get() {
    std::unique_lock<std::mutex> lock(_mutex);
    while (_length == 0) {
      _cond_busy.wait(lock);
    }
    if (_capacity - _length == 1) {
      _cond_idel.notify_one();
    }
    auto task = _queue[_head];
    move(_head);
    _length--;
    return task;
  }

  inline uint32_t move(uint32_t &index) {
    ++index;
    index %= _capacity;
    return index;
  }

private:
  uint32_t _capacity;
  uint32_t _head = 0;
  uint32_t _tail = 0;
  uint32_t _length = 0;
  std::vector<Task *> _queue;
  std::mutex _mutex;
  std::condition_variable _cond_busy;
  std::condition_variable _cond_idel;
};

class TaskPool {
public:
  TaskPool(int pool_size)
      : _pool_size(pool_size), _queue(pool_size * 4),
        _started(false), _done(false) {}
  ~TaskPool();

  void startPool();
  void addTask(Task *task) { _queue.put(task); }
  void waitTask(Task *task);
  void workFunc();

private:
  void addTerminateTask() { _queue.put(nullptr); }
  static void run(TaskPool *pool) { pool->workFunc(); }

  int _pool_size;
  RingQueue _queue;
  std::atomic<bool> _started;
  std::atomic<bool> _done;
  std::mutex _mutex;
  std::vector<std::thread> _threads;
  std::condition_variable _cond_feedback;
};

}
}

#endif // WORKQUEUE_threadpool_hpp