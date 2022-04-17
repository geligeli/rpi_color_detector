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
  void Spill();
  void Stop();
 private:
  bool DoOperation();
  std::atomic<int> nextOp;
  std::atomic<bool> finished;
  std::thread t;

};
}  // namespace stepper_thread