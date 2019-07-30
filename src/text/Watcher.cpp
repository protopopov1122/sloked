#include "sloked/text/Watcher.h"
#include <algorithm>
#include <iostream>

namespace sloked {

    TextWatcherBlock::TextWatcherBlock(TextBlock &text)
        : text(text) {}

    std::size_t TextWatcherBlock::GetLastLine() const {
        return this->text.GetLastLine();
    }

    std::size_t TextWatcherBlock::GetTotalLength() const {
        return this->text.GetTotalLength();
    }

    std::string_view TextWatcherBlock::GetLine(std::size_t idx) const {
        return this->text.GetLine(idx);
    }

    bool TextWatcherBlock::Empty() const {
        return this->text.Empty();
    }

    void TextWatcherBlock::Visit(std::size_t from, std::size_t to, Visitor v) const {
        this->text.Visit(from, to, std::move(v));
    }
    
    void TextWatcherBlock::SetLine(std::size_t idx, std::string_view view) {
        TextEditEvent evt {
            TextEditEvent::Update {idx, view}
        };
        this->Trigger(evt);
        this->text.SetLine(idx, view);
    }

    void TextWatcherBlock::EraseLine(std::size_t idx) {
        TextEditEvent evt {
            TextEditEvent::Erase {idx}
        };
        this->Trigger(evt);
        this->text.EraseLine(idx);
    }

    void TextWatcherBlock::InsertLine(std::size_t idx, std::string_view view) {
        TextEditEvent evt {
            TextEditEvent::Insert {idx, view}
        };
        this->Trigger(evt);
        this->text.InsertLine(idx, view);
    }

    void TextWatcherBlock::Optimize() {
        this->text.Optimize();
    }

    void TextWatcherBlock::AddListener(std::shared_ptr<TextEditListener> l) {
        this->listeners.push_back(l);
    }

    void TextWatcherBlock::RemoveListener(TextEditListener &l) {
        std::remove_if(this->listeners.begin(), this->listeners.end(), [&](const auto &listener) {
            return listener.get() == &l;
        });
    }

    void TextWatcherBlock::ClearListeners() {
        this->listeners.clear();
    }

    std::ostream &operator<<(std::ostream &os, const TextWatcherBlock &watcher) {
        return os << watcher.text;
    }

    void TextWatcherBlock::Trigger(const TextEditEvent &evt) {
        for (auto &l : this->listeners) {
            l->Trigger(evt);
        }
    }
}