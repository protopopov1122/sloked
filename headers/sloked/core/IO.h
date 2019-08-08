#ifndef SLOKED_CORE_IO_H_
#define SLOKED_CORE_IO_H_

#include "sloked/Base.h"
#include <cinttypes>
#include <string>

namespace sloked {

    class SlokedBaseIO {
     public:
        using Char = int;
        using Offset = uint64_t;
        enum class Origin {
            Begin = 0,
            Current,
            End
        };

        virtual ~SlokedBaseIO() = default;
        virtual void Close() = 0;
        virtual Offset Tell() = 0;
        virtual bool Seek(Offset, Origin) = 0;
        virtual bool HasErrors() = 0;
        virtual void ClearErrors() = 0;
    };

    class SlokedIOReader : public virtual SlokedBaseIO {
     public:
        virtual std::string Read(std::size_t) = 0;
        virtual Char Read() = 0;
        virtual bool Unread(Char) = 0;
        virtual bool Eof() = 0;
    };

    class SlokedIOWriter : public virtual SlokedBaseIO {
     public:
        virtual std::size_t Write(std::string_view) = 0;
        virtual bool Write(Char) = 0;
        virtual bool Flush() = 0;
    };

    class SlokedIOView {
     public:
        virtual ~SlokedIOView() = default;
        virtual std::string_view GetView() const = 0;
        operator std::string_view() const;
    };
}

#endif