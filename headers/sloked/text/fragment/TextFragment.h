#ifndef SLOKED_TEXT_FRAGMENT_TEXTFRAGMENT_H_
#define SLOKED_TEXT_FRAGMENT_TEXTFRAGMENT_H_

#include "sloked/core/Position.h"

namespace sloked {

    template <typename T>
    class TaggedTextFragment {
     public:
        TaggedTextFragment(const TextPosition &start, const TextPosition &length, T &&tag)
            : start(start), length(length), tag(std::forward<T>(tag)) {
            this->end = TextPosition{this->start.line + this->length.line, this->start.column + this->length.column};
        }

        const TextPosition &GetStart() const {
            return this->start;
        }

        const TextPosition &GetLength() const {
            return this->length;
        }

        const TextPosition &GetEnd() const {
            return this->end;
        }

        const T &GetTag() const {
            return this->tag;
        }

        bool Includes(const TextPosition &pos) const {
            return (this->start < pos || this->start == pos) &&
                pos < this->end;
        }

        bool Overlaps(const TaggedTextFragment<T> &other) const {
            const TextPosition &selfEnd = this->GetEnd();
            const TextPosition &otherEnd = other.GetEnd();
            return (this->start <= other.start && other.start < selfEnd) ||
                (other.start <= this->start && this->start < otherEnd);
        }

        bool operator<(const TaggedTextFragment<T> &other) const {
            return this->start < other.start;
        }

     private:
        TextPosition start;
        TextPosition length;
        TextPosition end;
        T tag;
    };
}

#endif