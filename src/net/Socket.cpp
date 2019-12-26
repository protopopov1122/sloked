#include "sloked/net/Socket.h"

namespace sloked {

    const SlokedSocketAuthentication *SlokedSocket::GetAuthentication() const {
        return nullptr;
    }

    SlokedSocketAuthentication *SlokedSocket::GetAuthentication() {
        return nullptr;
    }
}