#ifndef SLOKED_EDITOR_EDITORDOCUMENTS_H_
#define SLOKED_EDITOR_EDITORDOCUMENTS_H_

#include "sloked/screen/components/ComponentHandle.h"

namespace sloked {

    class SlokedEditorDocuments {
     public:
        class DocumentWindow {
         public:
            virtual ~DocumentWindow() = default;
            virtual bool IsOpened() const = 0;
            virtual bool HasFocus() const = 0;
            virtual void SetFocus() const = 0;
            virtual SlokedComponentWindow::Id GetId() const = 0;
            virtual void Save() = 0;
            virtual void Save(const std::string &) = 0;
            virtual void Close() = 0;
        };

        virtual ~SlokedEditorDocuments() = default;
        virtual std::size_t GetWindowCount() = 0;
        virtual std::shared_ptr<DocumentWindow> GetWindow(SlokedComponentWindow::Id) = 0;
        virtual std::shared_ptr<DocumentWindow> GetFocus() = 0;
        virtual std::shared_ptr<DocumentWindow> New(const std::string &, const std::string &) = 0;
        virtual std::shared_ptr<DocumentWindow> Open(const std::string &, const std::string &, const std::string &) = 0;
        virtual void CloseAll() = 0;
    };
}

#endif