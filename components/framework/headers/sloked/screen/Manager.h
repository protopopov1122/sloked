#ifndef SLOKED_SCREEN_MANAGER_H_
#define SLOKED_SCREEN_MANAGER_H_

#include "sloked/Base.h"
#include <chrono>
#include <atomic>
#include <thread>
#include <list>
#include <mutex>
#include <condition_variable>

namespace sloked {

    class SlokedScreenManager {
     public:
        using Duration = std::chrono::system_clock::duration;
        class Renderable {
         public:
            virtual ~Renderable() = default;
            virtual void Render() = 0;
        };

        SlokedScreenManager();
        bool IsRunning() const;
        void Start(Duration);
        void Stop();
        void Repaint();
        void Attach(Renderable &);
        void Detach(Renderable &);

     private:
        void Run(Duration);

        std::atomic_bool running;
        std::thread worker;
        std::mutex mtx;
        std::condition_variable cv;
        std::list<std::reference_wrapper<Renderable>> renderables;
    };
}

#endif