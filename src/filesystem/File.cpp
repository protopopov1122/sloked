#include "sloked/filesystem/File.h"

namespace sloked {

    SlokedFile *SlokedFSObject::AsFile() {
        if (this->GetType() == Type::RegularFile) {
            return dynamic_cast<SlokedFile *>(this);
        } else {
            return nullptr;
        }
    }

    SlokedDirectory *SlokedFSObject::AsDirectory() {
        if (this->GetType() == Type::Directory) {
            return dynamic_cast<SlokedDirectory *>(this);
        } else {
            return nullptr;
        }
    }
}
