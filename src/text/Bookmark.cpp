#include "sloked/text/Bookmark.h"

namespace sloked {

    class TextBookmark::Listener : public TextEditListener {
     public:
        Listener(TextBookmark &bookmark)
            : bookmark(bookmark) {
        }

        void Trigger(const TextEditEvent &evt) override {
            switch (evt.event.index()) {
                case 0:
                    if (std::get<0>(evt.event).line == this->bookmark.line) {
                        this->bookmark.valid = false;
                    }
                    break;

                case 1: {
                    const TextEditEvent::Erase &erase = std::get<1>(evt.event);
                    if (erase.line == this->bookmark.line) {
                        this->bookmark.valid = false;
                    } else if (erase.line < this->bookmark.line) {
                        this->bookmark.line--;
                    }
                } break;

                case 2: {
                    const TextEditEvent::Insert &insert = std::get<2>(evt.event);
                    if (insert.line < this->bookmark.line) {
                        this->bookmark.line++;
                    }
                } break;
            }
        }

     private:
        TextBookmark &bookmark;
    };

    TextBookmark::TextBookmark(TextEventWatcher &watcher, TextPosition::Line line)
        : watcher(watcher), line(line), valid(true) {
        this->listener = std::make_shared<Listener>(*this);
        this->watcher.AddListener(this->listener);
    }
    
    TextBookmark::~TextBookmark() {
        this->watcher.RemoveListener(*this->listener);
    }

    bool TextBookmark::IsValid() const {
        return this->valid;
    }

    TextPosition::Line TextBookmark::GetLine() const {
        return this->line;
    }
}