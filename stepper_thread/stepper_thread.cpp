#include "stepper_thread/stepper_thread.h"

#include <pigpio.h>

#include <chrono>
#include <filesystem>
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
  AUTOSORT = 6,
  RECORD_POSITION_TRAINING_DATA =7,
};

int wrapAround(int v, int delta, int mod) {
  v += delta;
  v += (1 - v / mod) * mod;
  return v % mod;
}

StepperThread::StepperThread(image_task::ImageTask& image_task)
    : m_image_task{image_task} {
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

  m_next_op = stepper_thread::OPERATIONS::NOP;
  m_finished = false;

  m_thread = std::thread([this]() {
    while (DoOperation() && !m_finished)
      ;
  });
}

StepperThread::~StepperThread() {
  m_finished = true;
  m_thread.join();
}


/**
 * 360 degrees = 1600 steps. Holes are 18 degrees apart.
 * 2 steps = 1.8 deg.
 */
void StepperThread::Step(int v, std::chrono::microseconds stepTime) {
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
  m_step_position = wrapAround(m_step_position, v, 1600);
}

bool StepperThread::DoOperation() {
  switch (m_next_op.load()) {
    case stepper_thread::OPERATIONS::NOP:
      std::this_thread::sleep_for(10ms);
      break;
    case stepper_thread::OPERATIONS::KEY_A:
      m_next_op = stepper_thread::OPERATIONS::NOP;
      Step(80, 4ms);
      Step(10, 16ms);
      Step(-20, 16ms);
      Step(10, 16ms);
      break;
    case stepper_thread::OPERATIONS::KEY_D:
      m_next_op = stepper_thread::OPERATIONS::NOP;
      Step(-80, 4ms);
      Step(-10, 16ms);
      Step(20, 16ms);
      Step(-10, 16ms);
      break;
    case stepper_thread::OPERATIONS::KEY_Q:
      m_next_op = stepper_thread::OPERATIONS::NOP;
      Step(1, 20ms);
      break;
    case stepper_thread::OPERATIONS::KEY_E:
      m_next_op = stepper_thread::OPERATIONS::NOP;
      Step(-1, 20ms);
      break;
    case stepper_thread::OPERATIONS::SPILL:
      Step(-1, 20ms);
      break;
    case stepper_thread::OPERATIONS::AUTOSORT: {
      const auto msSinceEpoch =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count();
      bool classification = false;
      {
        m_image_task.get().WaitForNewCapture();
        auto autoReEnable = m_image_task.get().suspendCapture();

        classification = m_image_task.get().getClassification() > 0.5;
        const auto outDir =
            classification ? std::filesystem::path("/nfs/general/shared/KeyA")
                           : std::filesystem::path("/nfs/general/shared/KeyD");
        std::filesystem::create_directories(outDir);
        m_image_task.get().dumpJpegFile(
            (outDir / (std::to_string(msSinceEpoch) + "_.jpg")));
      }
      if (classification) {
        Step(80, 4ms);
        Step(10, 16ms);
        Step(-20, 16ms);
        Step(10, 16ms);
      } else {
        Step(-80, 4ms);
        Step(-10, 16ms);
        Step(20, 16ms);
        Step(-10, 16ms);
      }
      break;
    }
    case stepper_thread::OPERATIONS::RECORD_POSITION_TRAINING_DATA: {
      const auto msSinceEpoch =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count();
      {
        m_image_task.get().WaitForNewCapture();
        auto autoReEnable = m_image_task.get().suspendCapture();
        const auto outDir = std::filesystem::path("/nfs/general/shared/pos/" + std::to_string(m_step_position));
        std::filesystem::create_directories(outDir);
        m_image_task.get().dumpJpegFile(
            (outDir / (std::to_string(msSinceEpoch) + "_.jpg")));
      }
      Step(m_record_position_direction, 20ms);
      if (m_step_position == 0) {
        m_record_position_direction *= -1;
      }
      break;
    }
    default:
      return false;
  }
  return true;
}

void StepperThread::KeyA() { m_next_op = stepper_thread::OPERATIONS::KEY_A; }
void StepperThread::KeyD() { m_next_op = stepper_thread::OPERATIONS::KEY_D; }
void StepperThread::KeyQ() { m_next_op = stepper_thread::OPERATIONS::KEY_Q; }
void StepperThread::KeyE() { m_next_op = stepper_thread::OPERATIONS::KEY_E; }
void StepperThread::Spill() { m_next_op = stepper_thread::OPERATIONS::SPILL; }
void StepperThread::Stop() { m_next_op = stepper_thread::OPERATIONS::NOP; }
void StepperThread::AutoSort() {
  m_next_op = stepper_thread::OPERATIONS::AUTOSORT;
}
void StepperThread::RecordPositionTrainingData() {
  m_next_op = stepper_thread::OPERATIONS::RECORD_POSITION_TRAINING_DATA;
}

}  // namespace stepper_thread