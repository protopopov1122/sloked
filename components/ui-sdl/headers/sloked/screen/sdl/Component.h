#ifndef SLOKED_SCREEN_SDL_COMPONENT_H_
#define SLOKED_SCREEN_SDL_COMPONENT_H_

#include "sloked/screen/sdl/Surface.h"

namespace sloked {

    class SlokedSDLComponent {
     public:
        virtual ~SlokedSDLComponent() = default;
        virtual void PollEvents(SlokedSDLEventQueue &) = 0;
        virtual void Render(SlokedSDLSurface &) = 0;
    };
}

#endif