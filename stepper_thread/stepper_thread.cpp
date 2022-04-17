#include "stepper_thread/stepper_thread.h"

#include <pigpio.h>

#include <chrono>
#include <iostream>

using namespace std::literals::chrono_literals;

namespace stepper_thread {

enum OPERATIONS : int {
  NOP = 0,
  KEY_A = 1,
  KEY_D = 2,
  KEY_Q = 3,
  KEY_E = 4,
  SPILL = 5,
  STOP_SPILL = 6,
};

namespace {
/**
 * 360 degrees = 1600 steps. Holes are 18 degrees apart.
 * 2 steps = 1.8 deg.
 */
void Step(int v, std::chrono::microseconds stepTime) {
  if (v < 0) {
    gpioWrite(24, 1);
  } else {
    gpioWrite(24, 0);
  }

  for (int i = 0; i < std::abs(v); ++i) {
    gpioWrite(23, 1);
    std::this_thread::sleep_for(stepTime / 2);
    gpioWrite(23, 0);
    std::this_thread::sleep_for(stepTime / 2);
  }
}
}  // namespace

StepperThread::StepperThread() {
  t = std::thread([this]() {
    while (DoOperation()) {
    }
  });
}

StepperThread::~StepperThread() { t.join(); }

bool StepperThread::DoOperation() {
  switch (nextOp.load()) {
    case stepper_thread::OPERATIONS::NOP:
      break;
    case stepper_thread::OPERATIONS::KEY_A:
      Step(80, 4ms);
      Step(10, 16ms);
      Step(-20, 16ms);
      Step(10, 16ms);
      break;
    case stepper_thread::OPERATIONS::KEY_D:
      Step(-80, 4ms);
      Step(-10, 16ms);
      Step(20, 16ms);
      Step(-10, 16ms);
      break;
    case stepper_thread::OPERATIONS::KEY_Q:
      Step(1);
      break;
    case stepper_thread::OPERATIONS::KEY_E:
      Step(-1);
      break;
    case stepper_thread::OPERATIONS::SPILL:
      Step(-1, 20ms);
      break;
    case stepper_thread::OPERATIONS::STOP_SPILL:
      nextOp = stepper_thread::OPERATIONS::NOP;
      break;
    default:
  }
  return true;
}

void StepperThread::KeyA() { nextOp = stepper_thread::OPERATIONS::KEY_A; }
void StepperThread::KeyD() { nextOp = stepper_thread::OPERATIONS::KEY_D; }
void StepperThread::KeyQ() { nextOp = stepper_thread::OPERATIONS::KEY_Q; }
void StepperThread::KeyE() { nextOp = stepper_thread::OPERATIONS::KEY_E; }
void StepperThread::ToggleSpill() {
  int expected = stepper_thread::OPERATIONS::SPILL;
  if (!std::atomic_compare_exchange_strong(
          nextOp, &expected, stepper_thread::OPERATIONS::STOP_SPILL)) {
    nextOp = stepper_thread::OPERATIONS::SPILL;
  }
}

}  // namespace stepper_thread