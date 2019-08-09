#ifndef SLOKED_FILESYSTEM_POSIX_VIEW_H_
#define SLOKED_FILESYSTEM_POSIX_VIEW_H_

#include "sloked/core/IO.h"

namespace sloked {

    class SlokedPosixFileView : public SlokedIOView {
     public:
        SlokedPosixFileView(FILE *);
        virtual ~SlokedPosixFileView();
        SlokedPosixFileView(const SlokedPosixFileView &) = delete;
        SlokedPosixFileView(SlokedPosixFileView &&) = default;

        SlokedPosixFileView &operator=(const SlokedPosixFileView &) = delete;
        SlokedPosixFileView &operator=(SlokedPosixFileView &&) = default;

        std::string_view GetView() const override;

     private:
        FILE *file;
        void *data;
        std::size_t length;
    };
}

#endif