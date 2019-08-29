#ifndef SLOKED_TEXT_FRAGMENT_UPDATER_H_
#define SLOKED_TEXT_FRAGMENT_UPDATER_H_

#include "sloked/text/cursor/TransactionStream.h"
#include "sloked/text/fragment/TaggedText.h"

namespace sloked {

    template <typename T>
    class SlokedFragmentUpdater : public SlokedTransactionStream::Listener {
    public:
        SlokedFragmentUpdater(const TextBlockView &text, SlokedTaggedText<T> &tags, const Encoding &encoding, SlokedCharWidth &charWidth)
            : text(text), tags(tags), encoding(encoding), charWidth(charWidth) {}

        void OnCommit(const SlokedCursorTransaction &trans) override {
            auto realPos = trans.GetPosition();
            TextPosition pos{realPos.line > 0 ? realPos.line - 1 : 0 , 0};
            this->tags.Rewind(pos);
        }

        void OnRollback(const SlokedCursorTransaction &trans) override {
            auto realPos = trans.GetPosition();
            TextPosition pos{realPos.line > 0 ? realPos.line - 1 : 0 , 0};
            this->tags.Rewind(pos);
        }

        void OnRevert(const SlokedCursorTransaction &trans) override {
            auto realPos = trans.GetPosition();
            TextPosition pos{realPos.line > 0 ? realPos.line - 1 : 0 , 0};
            this->tags.Rewind(pos);
        }

    private:
        const TextBlockView &text;
        SlokedTaggedText<T> &tags;
        const Encoding &encoding;
        SlokedCharWidth &charWidth;
    };
}

#endif