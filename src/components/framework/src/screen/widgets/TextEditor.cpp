/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as
  published by the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/screen/widgets/TextEditor.h"

#include "sloked/core/Locale.h"
#include "sloked/sched/Pipeline.h"
#include "sloked/screen/TaggedFrame.h"
#include "sloked/services/Cursor.h"

namespace sloked {

    SlokedTextEditor::SlokedTextEditor(
        const Encoding &encoding, const SlokedCharPreset &charPreset,
        std::unique_ptr<KgrPipe> cursorService,
        std::function<void(SlokedCursorClient &)> initClient,
        std::unique_ptr<KgrPipe> renderService,
        std::unique_ptr<KgrPipe> notifyService,
        SlokedEditorDocumentSet::DocumentId docId, const std::string &tagger,
        SlokedBackgroundGraphics bg, SlokedForegroundGraphics fg)
        : conv(encoding, SlokedLocale::SystemEncoding()),
          charPreset(charPreset), cursorClient(std::move(cursorService)),
          renderClient(std::move(renderService), docId),
          notifyClient(std::move(notifyService), docId), tagger(tagger),
          background(bg),
          foreground(fg), virtualCursorOffset{0, 0}, virtualCursor{0, 0},
          renderCache(
              [](const auto &, const auto &)
                  -> std::vector<SlokedTaggedTextFrame<bool>::TaggedLine> {
                  throw SlokedError("TextEditor: Unexpected cache miss");
              }),
          lifetime(std::make_shared<SlokedStandardLifetime>()) {
        if (initClient) {
            initClient(this->cursorClient);
        }
        notifyClient.OnUpdate([this](const auto &evt) {
            if (this->updateListener) {
                this->updateListener();
            }
        });
    }

    SlokedTextEditor::~SlokedTextEditor() {
        this->lifetime->Close();
    }

    bool SlokedTextEditor::ProcessInput(const SlokedKeyboardInput &cmd) {
        if (cmd.value.index() == 0) {
            this->cursorClient.Insert(conv.Convert(std::get<0>(cmd.value)));
        } else
            switch (std::get<1>(cmd.value)) {
                case SlokedControlKey::ArrowUp:
                    this->cursorClient.MoveUp();
                    if (this->updateListener) {
                        this->updateListener();
                    }
                    break;

                case SlokedControlKey::ArrowDown:
                    this->cursorClient.MoveDown();
                    if (this->updateListener) {
                        this->updateListener();
                    }
                    break;

                case SlokedControlKey::ArrowLeft:
                    this->cursorClient.MoveBackward();
                    if (this->updateListener) {
                        this->updateListener();
                    }
                    break;

                case SlokedControlKey::ArrowRight:
                    this->cursorClient.MoveForward();
                    if (this->updateListener) {
                        this->updateListener();
                    }
                    break;

                case SlokedControlKey::Enter:
                    this->cursorClient.NewLine();
                    break;

                case SlokedControlKey::Tab:
                    this->cursorClient.Insert("\t");
                    break;

                case SlokedControlKey::Backspace:
                    this->cursorClient.DeleteBackward();
                    break;

                case SlokedControlKey::Delete:
                    this->cursorClient.DeleteForward();
                    break;

                case SlokedControlKey::Home:
                    this->cursorClient.Undo();
                    break;

                case SlokedControlKey::End:
                    this->cursorClient.Redo();
                    break;

                default:
                    return false;
            }
        return true;
    }

    static SlokedTaggedTextFrame<bool>::TaggedLine ExtractTaggedLine(
        const KgrValue &raw) {
        SlokedTaggedTextFrame<bool>::TaggedLine taggedLine;
        const auto &fragments = raw.AsArray();
        for (const auto &fragment : fragments) {
            auto tag = fragment.AsDictionary()["tag"].AsBoolean();
            const auto &text = fragment.AsDictionary()["content"].AsString();
            const auto fragmentBegin = taggedLine.content.size();
            taggedLine.content.append(text);
            taggedLine.fragments.push_back({tag, fragmentBegin, text.size()});
        }
        return taggedLine;
    }

    template <typename Iter>
    static auto ExtractTaggedLines(const Iter &begin, const Iter &end) {
        std::vector<std::pair<TextPosition::Line,
                              SlokedTaggedTextFrame<bool>::TaggedLine>>
            result;
        for (auto it = begin; it != end; ++it) {
            result.emplace_back(
                std::make_pair(it->first, ExtractTaggedLine(it->second)));
        }
        return result;
    }

    static void UnwrapFragment(std::string_view in, const Encoding &encoding,
                               const SlokedCharPreset &charPreset,
                               const SlokedGraphemeEnumerator &graphemes,
                               TextPosition::Column &column, std::string &out) {
        graphemes.Iterate(
            encoding, in, [&](auto start, auto length, auto codepoints) {
                if (codepoints.Size() == 1 && codepoints[0] == U'\t') {
                    out.append(SlokedCharPreset::EncodeTab(charPreset, encoding,
                                                           column));
                    column += charPreset.GetCharWidth(U'\t', column);
                } else {
                    out.append(in.substr(start, length));
                    column++;
                }
                return true;
            });
    }

    static auto UnwrapTaggedLine(
        const SlokedTaggedTextFrame<bool>::TaggedLine &line,
        const Encoding &encoding, const SlokedCharPreset &charPreset,
        const SlokedGraphemeEnumerator &graphemes) {
        std::string_view view{line.content};
        SlokedTaggedTextFrame<bool>::TaggedLine taggedLine;
        TextPosition::Column column{0};
        for (const auto &fragment : line.fragments) {
            const auto fragmentBegin = taggedLine.content.size();
            UnwrapFragment(view.substr(fragment.offset, fragment.length),
                           encoding, charPreset, graphemes, column,
                           taggedLine.content);
            taggedLine.fragments.push_back(
                {fragment.tag, fragmentBegin,
                 taggedLine.content.size() - fragmentBegin});
        }
        return taggedLine;
    }

    static auto UpdateCursorLineOffset(TextPosition::Line offset,
                                       TextPosition::Line height,
                                       TextPosition::Line line) {
        if (offset + height - 1 < line) {
            offset = line - height + 1;
        }
        if (line < offset) {
            offset = line;
        }
        return offset;
    }

    struct GraphemePosition {
        TextPosition::Column virtualOffset;
        TextPosition::Column virtualLength;
        TextPosition::Column codepointOffset;
        TextPosition::Column codepointCount;
        SlokedGraphicsPoint::Coordinate graphicalWidth;
    };

    template <typename Iter>
    static std::optional<GraphemePosition> FindGraphemeByCodepointOffset(
        const Iter &begin, const Iter &end,
        TextPosition::Column codepointOffset) {
        for (auto it = begin; it != end; ++it) {
            if (codepointOffset >= it->codepointOffset &&
                codepointOffset < it->codepointOffset + it->codepointCount) {
                return *it;
            }
        }
        return {};
    }

    static std::vector<GraphemePosition> SplitIntoGraphemes(
        std::string_view in, const Encoding &encoding,
        const SlokedCharPreset &charPreset,
        const SlokedGraphemeEnumerator &graphemes,
        const SlokedFontProperties &fontProperties) {

        std::vector<GraphemePosition> result;
        TextPosition::Column column{0};
        TextPosition::Column codepointOffset{0};
        graphemes.Iterate(
            encoding, in, [&](auto start, auto length, auto codepoints) {
                if (codepoints.Size() == 1 && codepoints[0] == U'\t') {
                    auto tabWidth = charPreset.GetCharWidth(U'\t', column);
                    const char32_t Space{U' '};
                    while (tabWidth-- > 0) {
                        result.push_back(GraphemePosition{
                            column, 1, codepointOffset, static_cast<TextPosition::Column>(codepoints.Size()),
                            fontProperties.GetWidth(SlokedSpan(&Space, 1))});
                        column++;
                    }
                } else {
                    result.push_back(GraphemePosition{
                        column,
                        1,
                        codepointOffset,
                        static_cast<TextPosition::Column>(codepoints.Size()),
                        fontProperties.GetWidth(codepoints)});
                    column++;
                }
                codepointOffset += codepoints.Size();
                return true;
            });
        return result;
    }

    template <typename Iter>
    static auto SumGraphemeWidth(const Iter &begin, const Iter &end,
                                 TextPosition::Column from,
                                 TextPosition::Column to) {
        SlokedGraphicsPoint::Coordinate total{0};
        for (auto it = begin; it != end; ++it) {
            if (from <= it->virtualOffset + it->virtualLength &&
                to >= it->virtualOffset) {
                total += it->graphicalWidth;
            }
        }
        return total;
    }

    static auto CalculateCursorColumnOffset(
        TextPosition::Column virtualOffset,
        SlokedGraphicsPoint::Coordinate maxWidth,
        TextPosition::Column codepointColumn, std::string_view line,
        const Encoding &encoding, const SlokedCharPreset &charPreset,
        const SlokedGraphemeEnumerator &graphemes,
        const SlokedFontProperties &fontProperties) {

        const auto graphemeList = SplitIntoGraphemes(line, encoding, charPreset,
                                                     graphemes, fontProperties);
        const auto currentGrapheme = FindGraphemeByCodepointOffset(
            graphemeList.begin(), graphemeList.end(), codepointColumn);
        const auto virtualColumn =
            currentGrapheme.has_value()
                ? currentGrapheme->virtualOffset
                : (graphemeList.empty()
                       ? 0
                       : graphemeList.back().virtualOffset +
                             graphemeList.back().virtualLength);
        if (virtualOffset > virtualColumn) {
            virtualOffset = virtualColumn;
        }
        while (SumGraphemeWidth(graphemeList.begin(), graphemeList.end(),
                                virtualOffset, virtualColumn) > maxWidth) {
            virtualOffset++;
        }
        return std::make_pair(virtualOffset, virtualColumn);
    }

    TaskResult<void> SlokedTextEditor::RenderSurface(
        SlokedGraphicsPoint::Coordinate maxWidth, TextPosition::Line height,
        const SlokedFontProperties &fontProperties) {
        struct State {
            State(SlokedTextEditor *self,
                  SlokedGraphicsPoint::Coordinate maxWidth,
                  TextPosition::Line height,
                  const SlokedFontProperties &fontProperties)
                : self(self), maxWidth(maxWidth), height(height),
                  fontProperties(fontProperties), codepointCursor{0, 0},
                  virtualCursor{0, 0},
                  virtualCursorOffset(self->virtualCursorOffset) {}

            SlokedTextEditor *self;
            const SlokedGraphicsPoint::Coordinate maxWidth;
            const TextPosition::Line height;
            const SlokedFontProperties &fontProperties;

            TextPosition codepointCursor;
            TextPosition virtualCursor;
            TextPosition virtualCursorOffset;
            std::vector<SlokedTaggedTextFrame<bool>::TaggedLine> rendered;
        };

        static const SlokedAsyncTaskPipeline Pipeline(
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<State> &state,
                   std::optional<TextPosition> cursor) {
                    if (!cursor.has_value()) {
                        throw SlokedTaskPipelineStages::Cancel{};
                    }
                    state->codepointCursor = cursor.value();
                    state->virtualCursorOffset.line = UpdateCursorLineOffset(
                        state->virtualCursorOffset.line, state->height,
                        state->codepointCursor.line);
                    return state->self->renderClient.PartialRender(
                        state->virtualCursorOffset.line, state->height - 1);
                }),
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<State> &state,
                   std::tuple<
                       TextPosition::Line, TextPosition::Line,
                       std::vector<std::pair<TextPosition::Line, KgrValue>>>
                       result) {
                    auto updatedTaggedLines = ExtractTaggedLines(
                        std::get<2>(result).begin(), std::get<2>(result).end());
                    state->self->renderCache.Insert(updatedTaggedLines.begin(),
                                                    updatedTaggedLines.end());
                    const auto &render = state->self->renderCache.Fetch(
                        std::get<0>(result), std::get<1>(result));
                    state->rendered.clear();
                    state->rendered.reserve(state->height);
                    for (auto it = render.first; it != render.second; ++it) {
                        state->rendered.push_back(it->second);
                    }

                    state->virtualCursor.line = state->codepointCursor.line;
                    const auto &currentTaggedLine =
                        state->self->renderCache.Get(state->virtualCursor.line);
                    SlokedGraphemeNullEnumerator graphemeIter;
                    std::tie(state->virtualCursorOffset.column,
                             state->virtualCursor.column) =
                        CalculateCursorColumnOffset(
                            state->virtualCursorOffset.column, state->maxWidth,
                            state->codepointCursor.column,
                            currentTaggedLine.content,
                            state->self->conv.GetDestination(),
                            state->self->charPreset, graphemeIter,
                            state->fontProperties);

                    state->self->virtualCursorOffset =
                        std::move(state->virtualCursorOffset);
                    state->self->virtualCursor =
                        std::move(state->virtualCursor);
                    state->self->rendered = std::move(state->rendered);
                }),
            SlokedTaskPipelineStages::Catch(
                [](const std::shared_ptr<State> &, const std::exception_ptr &) {
                    // Skip
                }),
            SlokedTaskPipelineStages::MapCancelled(
                [](const std::shared_ptr<State> &) {
                    // Skip
                }));
        return Pipeline(
            std::make_shared<State>(this, maxWidth, height, fontProperties),
            this->cursorClient.GetPosition(), this->lifetime);
    }

    void SlokedTextEditor::ShowSurface(SlokedTextPane &pane) {
        pane.SetGraphicsMode(SlokedTextGraphics::Off);
        pane.SetGraphicsMode(this->background);
        pane.SetGraphicsMode(this->foreground);
        pane.ClearScreen();
        pane.SetPosition(0, 0);

        TextPosition::Line lineIdx{0};
        SlokedGraphemeNullEnumerator graphemeIter;
        SlokedTaggedTextFrame<bool> visualFrame(this->conv.GetDestination(),
                                                graphemeIter,
                                                pane.GetFontProperties());
        for (const auto &line : this->rendered) {
            pane.SetPosition(lineIdx++, 0);
            auto unwrappedLine =
                UnwrapTaggedLine(line, this->conv.GetDestination(),
                                 this->charPreset, graphemeIter);
            auto result = visualFrame.Slice(unwrappedLine,
                                            this->virtualCursorOffset.column,
                                            pane.GetMaxWidth());
            for (const auto &fragment : result.fragments) {
                if (fragment.tag) {
                    pane.SetGraphicsMode(SlokedBackgroundGraphics::Blue);
                } else {
                    pane.SetGraphicsMode(SlokedTextGraphics::Off);
                    pane.SetGraphicsMode(this->background);
                    pane.SetGraphicsMode(this->foreground);
                }

                auto view = std::string_view{result.content}.substr(
                    fragment.offset, fragment.length);
                pane.Write(this->conv.ReverseConvert(view));
            }
        }

        pane.SetPosition(
            this->virtualCursor.line - this->virtualCursorOffset.line,
            this->virtualCursor.column - this->virtualCursorOffset.column);
    }

    void SlokedTextEditor::OnUpdate(std::function<void()> listener) {
        this->updateListener = listener;
    }
}  // namespace sloked