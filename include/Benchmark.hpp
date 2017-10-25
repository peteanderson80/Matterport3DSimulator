#ifndef MATTERSIM_BENCHMARK
#define MATTERSIM_BENCHMARK

#include <chrono>

namespace mattersim {

    class Timer {
    public:
        Timer();
        virtual void Start();
        virtual void Stop();
        virtual void Reset();
        virtual float MilliSeconds();
        virtual float MicroSeconds();
        virtual float Seconds();
        inline bool running() { return running_; }

    protected:
        bool running_;
        std::chrono::steady_clock::time_point start_;
        std::chrono::steady_clock::duration elapsed_;
    };
}

#endif   // MATTERSIM_BENCHMARK
