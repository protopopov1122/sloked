#include "sloked/namespace/Object.h"

namespace sloked {

    SlokedNSDocument *SlokedNamespaceObject::AsDocument() {
        if (this->GetType() == Type::Document) {
            return dynamic_cast<SlokedNSDocument *>(this);
        } else {
            return nullptr;
        }
    }

    SlokedNamespace *SlokedNamespaceObject::AsNamespace() {
        if (this->GetType() == Type::Namespace) {
            return dynamic_cast<SlokedNamespace *>(this);
        } else {
            return nullptr;
        }
    }
}