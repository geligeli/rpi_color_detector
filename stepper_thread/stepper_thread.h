#pragma once

#include <atomic>
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
  void RecordPositionTrainingData();

 private:
  void Step(int v, std::chrono::microseconds stepTime);
  bool DoOperation();
  std::reference_wrapper<image_task::ImageTask> m_image_task;
  std::atomic<int> m_next_op;
  std::atomic<bool> m_finished;
  std::thread m_thread;
  int m_step_position{};
  enum DIRECTION : int {
    LEFT=1,RIGHT=-1
  };
  DIRECTION m_record_position_direction{LEFT};
};
}  // namespace stepper_thread