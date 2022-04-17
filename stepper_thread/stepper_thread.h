#pragma once

#include <thread>
#include <atomic>
#include <functional>


namespace stepper_thread {
class StepperThread {
 public:
  StepperThread(std::function<float()> getCurrentClassification);
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
  std::atomic<int> nextOp;
  std::atomic<bool> finished;
  std::thread t;
  std::function<float()> curClass;
};
}  // namespace stepper_thread