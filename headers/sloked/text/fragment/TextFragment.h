#ifndef SLOKED_TEXT_FRAGMENT_TEXTFRAGMENT_H_
#define SLOKED_TEXT_FRAGMENT_TEXTFRAGMENT_H_

#include "sloked/core/Position.h"

namespace sloked {

    template <typename T>
    class TaggedTextFragment {
     public:
        TaggedTextFragment(const TextPosition &start, const TextPosition &length, T &&tag)
            : start(start), length(length), tag(std::forward<T>(tag)) {}

        const TextPosition &GetStart() const {
            return this->start;
        }

        const TextPosition &GetLength() const {
            return this->length;
        }

        TextPosition GetEnd() const {
            return TextPosition{this->start.line + this->length.line, this->start.column + this->length.column};
        }

        const T &GetTag() const {
            return this->tag;
        }

        bool Includes(const TextPosition &pos) const {
            TextPosition end = this->GetEnd();
            return (start < pos || start == pos) &&
                pos < end;
        }

        bool Overlaps(const TaggedTextFragment<T> &other) const {
            TextPosition selfEnd = this->GetEnd();
            TextPosition otherEnd{other.start.line + other.length.line, other.start.column + other.length.column};
            return (this->start <= other.start && other.start < selfEnd) ||
                (other.start <= this->start && this->start < otherEnd);
        }

        bool operator<(const TaggedTextFragment<T> &other) const {
            return this->start < other.start;
        }

     private:
        TextPosition start;
        TextPosition length;
        T tag;
    };
}

#endif