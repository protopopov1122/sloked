#include "sloked/screen/graphics/Compat.h"

#ifdef SLOKED_GFX_CAIRO_SDL
#include "sloked/screen/cairo/GUI.h"

namespace sloked {

    std::unique_ptr<SlokedGraphicalComponents> SlokedGraphicsCompat::GetGraphics(SlokedScreenManager &screenManager) {
        return std::make_unique<SlokedCairoSDLGraphicalComponents>(screenManager);
    }
}

#else
#include "sloked/core/Error.h"

namespace sloked {

    std::unique_ptr<SlokedGraphicalComponents> SlokedGraphicsCompat::GetGraphics(SlokedScreenManager &screenManager) {
        throw SlokedError("GraphicsCompat: Graphics not supported");
    }
}

#endif