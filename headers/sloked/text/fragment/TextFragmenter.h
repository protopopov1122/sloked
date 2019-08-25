#ifndef SLOKED_TEXT_FRAGMENT_TEXTFRAGMENTER_H_
#define SLOKED_TEXT_FRAGMENT_TEXTFRAGMENTER_H_

#include "sloked/text/fragment/FragmentMap.h"
#include <optional>

namespace sloked {

    template <typename T>
    class SlokedTextTagger {
     public:
        virtual ~SlokedTextTagger() = default;
        virtual std::optional<TaggedTextFragment<T>> Next() = 0;
        virtual void Rewind(const TextPosition &) = 0;
    };

    template <typename T>
    class SlokedTaggedText {
     public:
        SlokedTaggedText(std::unique_ptr<SlokedTextTagger<T>> tagger)
            : tagger(std::move(tagger)), current{0, 0} {
            auto fragment = this->tagger->Next();
            if (fragment.has_value()) {
                this->fragments.Insert(std::move(fragment.value()));
                this->current = fragment.value().GetEnd();
            }
        }

        const TaggedTextFragment<T> *Get(const TextPosition &pos) {
            while (!(pos < this->current)) {
                auto fragment = this->tagger->Next();
                if (fragment.has_value()) {
                    this->fragments.Insert(std::move(fragment.value()));
                    this->current = fragment.value().GetEnd();
                } else {
                    break;
                }
            }
            return this->fragments.Get(pos);
        }

        void Rewind(const TextPosition &pos) {
            auto start = this->fragments.NearestLeast(pos).value_or(TextPosition{0, 0});
            this->fragments.Remove(start);
            this->tagger->Rewind(start);
            this->current = start;
        }
    
     private:
        std::unique_ptr<SlokedTextTagger<T>> tagger;
        TaggedFragmentMap<T> fragments;
        TextPosition current;
    };
}

#endif