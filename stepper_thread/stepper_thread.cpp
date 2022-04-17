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
  SPILL = 5
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
  // Note this binds to port 8888!!
  if (gpioInitialise() < 0) {
    // pigpio initialisation failed.
    std::cerr << "pigpio initialisation failed.\n";
    std::terminate();
  }
  gpioSetMode(23, PI_OUTPUT);  // Set GPIO17 as input.
  gpioSetMode(24, PI_OUTPUT);  // Set GPIO18 as output.
  gpioWrite(23, 0);
  gpioWrite(24, 0);

  nexstOp = stepper_thread::OPERATIONS::NOP;
  finished = false;

  t = std::thread([this]() {
    while (DoOperation() && !finished)
      ;
  });
}

StepperThread::~StepperThread() {
  finished = true;
  t.join();
}

bool StepperThread::DoOperation() {
  switch (nextOp.load()) {
    case stepper_thread::OPERATIONS::NOP:
      std::this_thread::sleep_for(10ms);
      break;
    case stepper_thread::OPERATIONS::KEY_A:
      nextOp = stepper_thread::OPERATIONS::NOP;
      Step(80, 4ms);
      Step(10, 16ms);
      Step(-20, 16ms);
      Step(10, 16ms);
      break;
    case stepper_thread::OPERATIONS::KEY_D:
      nextOp = stepper_thread::OPERATIONS::NOP;
      Step(-80, 4ms);
      Step(-10, 16ms);
      Step(20, 16ms);
      Step(-10, 16ms);
      break;
    case stepper_thread::OPERATIONS::KEY_Q:
      nextOp = stepper_thread::OPERATIONS::NOP;
      Step(1, 20ms);
      break;
    case stepper_thread::OPERATIONS::KEY_E:
      nextOp = stepper_thread::OPERATIONS::NOP;
      Step(-1, 20ms);
      break;
    case stepper_thread::OPERATIONS::SPILL:
      Step(-1, 20ms);
      break;
    default:
      return false;
  }
  return true;
}

void StepperThread::KeyA() { nextOp = stepper_thread::OPERATIONS::KEY_A; }
void StepperThread::KeyD() { nextOp = stepper_thread::OPERATIONS::KEY_D; }
void StepperThread::KeyQ() { nextOp = stepper_thread::OPERATIONS::KEY_Q; }
void StepperThread::KeyE() { nextOp = stepper_thread::OPERATIONS::KEY_E; }
void StepperThread::Spill() { nextOp = stepper_thread::OPERATIONS::SPILL; }
void StepperThread::Stop() { nextOp = stepper_thread::OPERATIONS::NOP; }

}  // namespace stepper_thread