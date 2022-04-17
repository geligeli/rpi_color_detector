#pragma once

#include <thread>
#include <atomic>


namespace stepper_thread {
class StepperThread {
 public:
  StepperThread();
  ~StepperThread();
  void KeyA();
  void KeyD();
  void KeyQ();
  void KeyE();
  void ToggleSpill();
 private:
  bool DoOperation();
  std::atomic<int> nextOp;
  std::thread t;

};
}  // namespace stepper_thread