#include "sloked/filesystem/FileIO.h"

namespace sloked {

    SlokedFileView::operator std::string_view() const {
        return this->GetView();
    }
}