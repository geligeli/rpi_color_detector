#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <thread>

#include "image_task/image_task.h"

namespace stepper_thread {
class StepperThread {
 public:
  StepperThread(image_task::ImageTask&);
  ~StepperThread();
  void KeyA();
  void KeyD();
  void KeyQ();
  void KeyE();
  void Spill();
  void Stop();
  void AutoSort();

 private:
  bool DoOperation();
  std::reference_wrapper<image_task::ImageTask> m_image_task;
  std::atomic<int> m_next_op;
  std::atomic<bool> m_finished;
  std::thread m_thread;
};
}  // namespace stepper_thread