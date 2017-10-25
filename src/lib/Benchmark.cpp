#include <chrono>

#include "Benchmark.hpp"

namespace mattersim {

    Timer::Timer()
        : running_(false),
          elapsed_(0) {}

    void Timer::Start() {
      if (!running()) {
        start_ = std::chrono::steady_clock::now();
        running_ = true;
      }
    }

    void Timer::Stop() {
      if (running()) {
        elapsed_ += std::chrono::steady_clock::now() - start_;
        running_ = false;
      }
    }

    void Timer::Reset() {
      if (running()) {
        running_ = false;
      }
      elapsed_ = std::chrono::steady_clock::duration(0);
    }

    float Timer::MicroSeconds() {
      return std::chrono::duration_cast<std::chrono::microseconds>(elapsed_).count();
    }

    float Timer::MilliSeconds() {
      return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_).count();
    }

    float Timer::Seconds() {
      return std::chrono::duration_cast<std::chrono::seconds>(elapsed_).count();
    }


}
