#include "sloked/editor/Application.h"

#include <iostream>

#include "sloked/kgr/Serialize.h"

namespace sloked {

    class SlokedHeadlessEditorApplication : public SlokedApplication {
     public:
        int Start(int, const char **, const SlokedBaseInterface &baseInterface,
                  SlokedSharedContainerEnvironment &sharedEnv,
                  SlokedEditorManager &manager) final {
            KgrJsonSerializer serializer;
            auto configuration = serializer.Deserialize(std::cin);
            manager.Spawn(configuration.AsDictionary()["containers"]);
            manager.GetTotalShutdown().WaitForShutdown();
            return EXIT_SUCCESS;
        }
    };

    extern "C" SlokedApplication *SlokedMakeApplication() {
        return new SlokedHeadlessEditorApplication();
    }
}  // namespace sloked