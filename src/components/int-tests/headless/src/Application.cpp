#include "sloked/editor/Application.h"

#include <iostream>

#include "sloked/kgr/Serialize.h"
#include "sloked/text/fragment/Iterator.h"
#include "sloked/text/fragment/TaggedText.h"
#include "sloked/text/fragment/Updater.h"

namespace sloked {
    class TestFragment
        : public SlokedTextTagIterator<SlokedEditorDocument::TagType> {
        class DocumentListener : public SlokedTransactionStreamListener {
         public:
            DocumentListener(
                SlokedEventEmitter<const TextPositionRange &> &emitter)
                : emitter(emitter) {}

            void OnCommit(const SlokedCursorTransaction &transaction) final {
                this->Emit(transaction.GetPosition().line);
            }

            void OnRollback(const SlokedCursorTransaction &transaction) final {
                this->Emit(transaction.GetPosition().line);
            }

            void OnRevert(const SlokedCursorTransaction &transaction) final {
                this->Emit(transaction.GetPosition().line);
            }

         private:
            void Emit(TextPosition::Line line) {
                if (line > 0) {
                    emitter.Emit(
                        {TextPosition{line - 1, 0}, TextPosition::Max});
                } else {
                    emitter.Emit({TextPosition{0, 0}, TextPosition::Max});
                }
            }

            SlokedEventEmitter<const TextPositionRange &> &emitter;
        };

     public:
        TestFragment(const TextBlockView &text, const Encoding &encoding,
                     SlokedTransactionListenerManager &listeners)
            : text(text), encoding(encoding), current{0, 0},
              listeners(listeners) {
            this->listener = std::make_shared<DocumentListener>(this->emitter);
            this->listeners.AddListener(this->listener);
        }

        ~TestFragment() {
            this->listeners.RemoveListener(*this->listener);
        }

        std::optional<TaggedTextFragment<SlokedEditorDocument::TagType>> Next()
            override {
            if (this->cache.empty()) {
                this->ParseLine();
            }
            if (!this->cache.empty()) {
                auto fragment = std::move(this->cache.front());
                this->cache.pop();
                return fragment;
            } else {
                return {};
            }
        }

        void Rewind(const TextPosition &position) override {
            if (position < this->current) {
                this->current = position;
                this->cache = {};
                this->emitter.Emit(
                    TextPositionRange{position, TextPosition::Max});
            }
        }

        const TextPosition &GetPosition() const override {
            return this->current;
        }

        Unbind OnChange(
            std::function<void(const TextPositionRange &)> fn) final {
            return this->emitter.Listen(std::move(fn));
        }

     private:
        bool ParseLine() {
            if (this->current.line > this->text.GetLastLine()) {
                return false;
            }

            std::string currentLine{this->text.GetLine(this->current.line)};
            currentLine.erase(
                0,
                this->encoding.GetCodepoint(currentLine, this->current.column)->start);
            this->encoding.IterateCodepoints(
                currentLine, [&](auto start, auto length, auto chr) {
                    if (chr == U'\t') {
                        this->cache.push(
                            TaggedTextFragment<SlokedEditorDocument::TagType>(
                                this->current,
                                TextPosition{
                                    0, static_cast<TextPosition::Column>(1)},
                                1));
                    }
                    this->current.column++;
                    return true;
                });

            this->current.line++;
            this->current.column = 0;
            return true;
        }

        const TextBlockView &text;
        const Encoding &encoding;
        TextPosition current;
        std::queue<TaggedTextFragment<SlokedEditorDocument::TagType>> cache;
        SlokedEventEmitter<const TextPositionRange &> emitter;
        SlokedTransactionListenerManager &listeners;
        std::shared_ptr<DocumentListener> listener;
    };

    class SlokedTestTagger
        : public SlokedTextTagger<SlokedEditorDocument::TagType> {
     public:
        SlokedTestTagger(SlokedTaggableDocument &doc)
            : doc(doc), iter(doc.GetText(), doc.GetEncoding(),
                             doc.GetTransactionListeners()),
              updater(std::make_shared<
                      SlokedFragmentUpdater<SlokedEditorDocument::TagType>>(
                  doc.GetText(), iter, doc.GetEncoding())),
              lazy(iter), cached(lazy) {
            doc.GetTransactionListeners().AddListener(this->updater);
        }

        ~SlokedTestTagger() {
            doc.GetTransactionListeners().RemoveListener(*this->updater);
        }

        std::optional<TaggedTextFragment<SlokedEditorDocument::TagType>> Get(
            const TextPosition &pos) final {
            return this->cached.Get(pos);
        }

        std::vector<TaggedTextFragment<SlokedEditorDocument::TagType>> Get(
            TextPosition::Line line) final {
            return this->cached.Get(line);
        }

        typename SlokedTextTagger<SlokedEditorDocument::TagType>::Unbind
            OnChange(
                std::function<void(const TextPositionRange &)> callback) final {
            return this->cached.OnChange(std::move(callback));
        }

     private:
        SlokedTaggableDocument &doc;
        TestFragment iter;
        std::shared_ptr<SlokedFragmentUpdater<SlokedEditorDocument::TagType>>
            updater;
        SlokedLazyTaggedText<SlokedEditorDocument::TagType> lazy;
        SlokedCacheTaggedText<SlokedEditorDocument::TagType> cached;
    };

    class TestFragmentFactory
        : public SlokedTextTaggerFactory<SlokedEditorDocument::TagType> {
     public:
        std::unique_ptr<SlokedTextTagger<SlokedEditorDocument::TagType>> Create(
            SlokedTaggableDocument &doc) const final {
            return std::make_unique<SlokedTestTagger>(doc);
        }
    };

    class SlokedHeadlessEditorApplication : public SlokedApplication {
     public:
        int Start(int, const char **, const SlokedBaseInterface &baseInterface,
                  SlokedSharedContainerEnvironment &sharedEnv,
                  SlokedEditorManager &manager) final {
            KgrJsonSerializer serializer;
            auto configuration = serializer.Deserialize(std::cin);
            manager.GetBaseTaggers().Bind("default",
                                        std::make_unique<TestFragmentFactory>());
            manager.Spawn(configuration.AsDictionary()["containers"]);
            manager.GetTotalShutdown().WaitForShutdown();
            return EXIT_SUCCESS;
        }
    };

    extern "C" SlokedApplication *SlokedMakeApplication() {
        return new SlokedHeadlessEditorApplication();
    }
}  // namespace sloked