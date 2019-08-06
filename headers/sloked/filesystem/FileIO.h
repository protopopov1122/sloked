#ifndef SLOKED_FILESYSTEM_FILEIO_H_
#define SLOKED_FILESYSTEM_FILEIO_H_

#include "sloked/Base.h"
#include <cinttypes>
#include <string>

namespace sloked {

    class SlokedFileIO {
     public:
        using Char = int;
        using Offset = uint64_t;
        enum class Origin {
            Begin = 0,
            Current,
            End
        };

        virtual ~SlokedFileIO() = default;
        virtual void Close() = 0;
        virtual Offset Tell() = 0;
        virtual bool Seek(Offset, Origin) = 0;
        virtual bool HasErrors() = 0;
        virtual void ClearErrors() = 0;
    };

    class SlokedFileReader : public virtual SlokedFileIO {
     public:
        virtual std::string Read(std::size_t) = 0;
        virtual Char Read() = 0;
        virtual bool Unread(Char) = 0;
        virtual bool Eof() = 0;
    };

    class SlokedFileWriter : public virtual SlokedFileIO {
     public:
        virtual std::size_t Write(std::string_view) = 0;
        virtual bool Write(Char) = 0;
        virtual bool Flush() = 0;
    };

    class SlokedFileView {
     public:
        virtual ~SlokedFileView() = default;
        virtual std::string_view GetView() const = 0;
        operator std::string_view() const;
    };
}

#endif