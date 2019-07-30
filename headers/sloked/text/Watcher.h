#ifndef SLOKED_TEXT_WATCHER_H_
#define SLOKED_TEXT_WATCHER_H_

#include "sloked/Base.h"
#include "sloked/text/TextBlock.h"
#include <string>
#include <cinttypes>
#include <vector>
#include <memory>
#include <variant>

namespace sloked {

    struct TextEditEvent {
        struct Update {
            std::size_t line;
            std::string_view content;
        };

        struct Erase {
            std::size_t line;
        };

        struct Insert {
            std::size_t line;
            std::string_view content;
        };

        std::variant<Update, Erase, Insert> event;
    };

    class TextEditListener {
     public:
        virtual ~TextEditListener() = default;
        virtual void Trigger(const TextEditEvent &) = 0;
    };

    class TextEventWatcher {
     public:
        virtual ~TextEventWatcher() = default;
        virtual void AddListener(std::shared_ptr<TextEditListener>) = 0;
        virtual void RemoveListener(TextEditListener &) = 0;
        virtual void ClearListeners() = 0;
    };

    class TextWatcherBlock : public TextBlockImpl<TextWatcherBlock>, public TextEventWatcher {
     public:
        TextWatcherBlock(TextBlock &);

        std::size_t GetLastLine() const override;
        std::size_t GetTotalLength() const override;
        std::string_view GetLine(std::size_t) const override;
        bool Empty() const override;
        void Visit(std::size_t, std::size_t, Visitor) const;
        
        void SetLine(std::size_t, std::string_view) override;
        void EraseLine(std::size_t) override;
        void InsertLine(std::size_t, std::string_view) override;
        void Optimize() override;

        void AddListener(std::shared_ptr<TextEditListener>) override;
        void RemoveListener(TextEditListener &) override;
        void ClearListeners() override;

        friend std::ostream &operator<<(std::ostream &, const TextWatcherBlock &);

     private:
        void Trigger(const TextEditEvent &);

        TextBlock &text;
        std::vector<std::shared_ptr<TextEditListener>> listeners;
    };
}

#endif