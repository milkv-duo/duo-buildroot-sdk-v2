#include <sys/mman.h>
#include <unistd.h>
#include <iostream>
#include <mutex>
#include <runtime/taskpool.hpp>
#include <runtime/model.hpp>

namespace cvi {
namespace runtime {

Task::Task(TaskPool *pool, void *program, CVI_TENSOR *inputs,
           int input_num, CVI_TENSOR *outputs, int output_num)
    : program(program), input_num(input_num), output_num(output_num),
      inputs(inputs), outputs(outputs) {
  pool->addTask(this);
}

TaskPool::~TaskPool() {
  if (_started) {
    _done = true;
    for (int i = 0; i < _pool_size; ++i) {
      addTerminateTask();
    }
    for (auto &thread : _threads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }
}

void TaskPool::startPool() {
  if (_started) {
    return;
  }
  std::unique_lock<std::mutex> lock(_mutex);
  for (int i = 0; i < _pool_size; ++i) {
    _threads.push_back(std::thread(run, this));
  }
  while (!_started) {
    usleep(10);
  }
}

void TaskPool::workFunc() {
  _started = true;
  while (!_done) {
    auto task = _queue.get();
    if (task == nullptr)
      continue;
    auto program = (Program *)task->program;
    task->retCode = (CVI_RC)program->forward(task->inputs, task->input_num, task->outputs,
                                     task->output_num);
    std::unique_lock<std::mutex> lock(_mutex);
    _cond_feedback.notify_all();
  }
}

void TaskPool::waitTask(Task *task) {
  std::unique_lock<std::mutex> lock(_mutex);
  while (task->retCode == CVI_RC_UNINIT)
    _cond_feedback.wait(lock);
}

}
}
