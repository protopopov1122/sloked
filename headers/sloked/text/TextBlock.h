#ifndef SLOKED_TEXT_TEXTBLOCK_H_
#define SLOKED_TEXT_TEXTBLOCK_H_

#include "sloked/Base.h"
#include <string>
#include <memory>
#include <cinttypes>
#include <iosfwd>
#include <functional>

namespace sloked {

    class TextBlock {
     public:
        using Visitor = std::function<void(std::string_view)>;
        virtual ~TextBlock() = default;

        virtual std::size_t GetLastLine() const = 0;
        virtual std::size_t GetTotalLength() const = 0;
        virtual std::string_view GetLine(std::size_t) const = 0;
        virtual bool Empty() const = 0;
        virtual void Visit(std::size_t, std::size_t, Visitor) const;
        
        virtual void SetLine(std::size_t, std::string_view) = 0;
        virtual void EraseLine(std::size_t) = 0;
        virtual void InsertLine(std::size_t, std::string_view) = 0;
        virtual void Optimize();

        friend std::ostream &operator<<(std::ostream &os, const TextBlock &block) {
            return block.dump(os);
        }
        
     protected:
        virtual std::ostream &dump(std::ostream &) const = 0;
    };

    template <typename T>
    class TextBlockImpl : public TextBlock {
     protected:
        std::ostream &dump(std::ostream &os) const override {
            return os << *static_cast<const T *>(this);
        }
    };

    class TextBlockFactory {
     public:
        virtual ~TextBlockFactory() = default;
        virtual std::unique_ptr<TextBlock> make(std::string_view) const = 0;
    };
}

#endif