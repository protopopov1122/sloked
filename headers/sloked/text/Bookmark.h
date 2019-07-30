#ifndef SLOKED_TEXT_BOOKMARK_H_
#define SLOKED_TEXT_BOOKMARK_H_

#include "sloked/core/Position.h"
#include "sloked/text/Watcher.h"
#include <memory>

namespace sloked {

    class TextBookmark {
     public:
        TextBookmark(TextEventWatcher &, TextPosition::Line);
        ~TextBookmark();

        bool IsValid() const;
        TextPosition::Line GetLine() const;

        class Listener;
        friend class Listener;

     private:
        bool valid;
        TextEventWatcher &watcher;
        TextPosition::Line line;
        std::shared_ptr<Listener> listener;
    };
}

#endif