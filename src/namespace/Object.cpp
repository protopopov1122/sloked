#include "sloked/namespace/Object.h"

namespace sloked {

    SlokedNamespaceDocument *SlokedNamespaceObject::AsDocument() {
        if (this->GetType() == Type::Document) {
            return dynamic_cast<SlokedNamespaceDocument *>(this);
        } else {
            return nullptr;
        }
    }
}