#ifndef SLOKED_SCREEN_MANAGER_H_
#define SLOKED_SCREEN_MANAGER_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>

#include "sloked/Base.h"

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
}  // namespace sloked

#endif