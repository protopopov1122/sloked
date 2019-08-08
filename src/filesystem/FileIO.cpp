#include "sloked/core/IO.h"

namespace sloked {

    SlokedIOView::operator std::string_view() const {
        return this->GetView();
    }
}