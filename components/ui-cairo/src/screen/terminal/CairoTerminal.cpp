#include "sloked/screen/terminal/CairoTerminal.h"

namespace sloked {

    // SlokedCairoTerminal::SlokedCairoTerminal(Cairo::RefPtr<Cairo::Surface> surface, std::pair<int, int> sirfaceSize)
    //     : surface(surface), context(Cairo::Context::create(surface)),
    //     fontLayout{Pango::Layout::create(Glib::wrap(pango_font_map_create_context(pango_cairo_font_map_get_default())))},
    //     cursor{0, 0} {
    //     Pango::FontDescription fontDescription(pango_font_description_from_string("Monospace 12"));
    //     this->fontLayout->set_font_description(fontDescription);
    //     auto font = Glib::wrap(pango_font_map_load_font(pango_cairo_font_map_get_default(),
    //         this->fontLayout->get_context()->gobj(),
    //         fontDescription.gobj()));
    // }

    // void SetPosition(Line, Column) final;
    // void MoveUp(Line) final;
    // void MoveDown(Line) final;
    // void MoveBackward(Column) final;
    // void MoveForward(Column) final;

    // void ShowCursor(bool) final;
    // void ClearScreen() final;
    // void ClearChars(Column) final;
    // Column GetWidth() final;
    // Line GetHeight() final;

    // void Write(std::string_view) final;

    // void SetGraphicsMode(SlokedTextGraphics) final;
    // void SetGraphicsMode(SlokedBackgroundGraphics) final;
    // void SetGraphicsMode(SlokedForegroundGraphics) final;
}