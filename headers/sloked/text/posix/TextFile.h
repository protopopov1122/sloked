#ifndef SLOKED_TEXT_POSIX_TEXTFILE_H_
#define SLOKED_TEXT_POSIX_TEXTFILE_H_

#include "sloked/Base.h"
#include "sloked/core/NewLine.h"
#include "sloked/text/TextRegion.h"
#include "sloked/text/TextChunk.h"
#include <iosfwd>
#include <variant>

namespace sloked {

    class PosixTextView {
     public:
        PosixTextView(int);
        virtual ~PosixTextView();
        PosixTextView(const PosixTextView &) = delete;
        PosixTextView(PosixTextView &&) = default;

        PosixTextView &operator=(const PosixTextView &) = delete;
        PosixTextView &operator=(PosixTextView &&) = default;

        std::string_view GetView() const;
        operator std::string_view() const;

     private:
        int fd;
        void *data;
        std::size_t length;
    };
}

#endif