/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as published by
  the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/services/Screen.h"
#include "sloked/screen/components/ComponentTree.h"
#include "sloked/screen/components/MultiplexerComponent.h"
#include "sloked/screen/components/SplitterComponent.h"
#include "sloked/screen/components/TabberComponent.h"
#include "sloked/screen/widgets/TextEditor.h"

namespace sloked {

    class SlokedScreenContext : public SlokedServiceContext {
     public:
        SlokedScreenContext(std::unique_ptr<KgrPipe> pipe, SlokedSynchronized<SlokedScreenComponent &> &root, const Encoding &encoding,
            KgrServer::Connector cursorService, KgrServer::Connector renderService)
            : SlokedServiceContext(std::move(pipe)), root(root), encoding(encoding),
              cursorService(std::move(cursorService)), renderService(std::move(renderService)) {
            
            this->BindMethod("handle.newMultiplexer", &SlokedScreenContext::HandleNewMultiplexer);
            this->BindMethod("handle.newSplitter", &SlokedScreenContext::HandleNewSplitter);
            this->BindMethod("handle.newTabber", &SlokedScreenContext::HandleNewTabber);
            this->BindMethod("handle.newTextEditor", &SlokedScreenContext::HandleNewTextEditor);
            this->BindMethod("multiplexer.newWindow", &SlokedScreenContext::MultiplexerNewWindow);
            this->BindMethod("multiplexer.getInfo", &SlokedScreenContext::MultiplexerGetInfo);
            this->BindMethod("multiplexer.windowHasFocus", &SlokedScreenContext::MultiplexerWindowHasFocus);
            this->BindMethod("multiplexer.setFocus", &SlokedScreenContext::MultiplexerSetFocus);
            this->BindMethod("multiplexer.close", &SlokedScreenContext::MultiplexerClose);
            this->BindMethod("multiplexer.moveWindow", &SlokedScreenContext::MultiplexerMoveWindow);
            this->BindMethod("multiplexer.resizeWindow", &SlokedScreenContext::MultiplexerResizeWindow);
            this->BindMethod("splitter.newWindow", &SlokedScreenContext::SplitterNewWindow);
            this->BindMethod("splitter.insertWindow", &SlokedScreenContext::SplitterInsertWindow);
            this->BindMethod("splitter.getInfo", &SlokedScreenContext::SplitterGetInfo);
            this->BindMethod("splitter.windowHasFocus", &SlokedScreenContext::SplitterWindowHasFocus);
            this->BindMethod("splitter.setFocus", &SlokedScreenContext::SplitterSetFocus);
            this->BindMethod("splitter.close", &SlokedScreenContext::SplitterClose);
            this->BindMethod("splitter.updateWindowConstraints", &SlokedScreenContext::SplitterUpdateWindowConstraints);
            this->BindMethod("splitter.moveWindow", &SlokedScreenContext::SplitterMoveWindow);
            this->BindMethod("tabber.newWindow", &SlokedScreenContext::TabberNewWindow);
            this->BindMethod("tabber.insertWindow", &SlokedScreenContext::TabberInsertWindow);
            this->BindMethod("tabber.getInfo", &SlokedScreenContext::TabberGetInfo);
            this->BindMethod("tabber.windowHasFocus", &SlokedScreenContext::TabberWindowHasFocus);
            this->BindMethod("tabber.setFocus", &SlokedScreenContext::TabberSetFocus);
            this->BindMethod("tabber.close", &SlokedScreenContext::TabberClose);
            this->BindMethod("tabber.moveWindow", &SlokedScreenContext::TabberMoveWindow);
        }

     private:
        void HandleNewMultiplexer(const std::string &method, const KgrValue &params, Response &rsp) {
            const auto &path = params.AsString();
            root.Lock([&](auto &screen) {
                try {
                    auto &handle = SlokedComponentTree::Traverse(screen, SlokedPath{path}).AsHandle();
                    handle.NewMultiplexer();
                    rsp.Result(true);
                } catch (const SlokedError &err) {
                    rsp.Result(false);
                }
            });
        }

        void HandleNewSplitter(const std::string &method, const KgrValue &params, Response &rsp) {
            const auto &path = params.AsDictionary()["path"].AsString();
            auto direction = static_cast<Splitter::Direction>(params.AsDictionary()["direction"].AsInt());
            root.Lock([&](auto &screen) {
                try {
                    auto &handle = SlokedComponentTree::Traverse(screen, SlokedPath{path}).AsHandle();
                    handle.NewSplitter(direction);
                    rsp.Result(true);
                } catch (const SlokedError &err) {
                    rsp.Result(false);
                }
            });
        }

        void HandleNewTabber(const std::string &method, const KgrValue &params, Response &rsp) {
            const auto &path = params.AsString();
            root.Lock([&](auto &screen) {
                try {
                    auto &handle = SlokedComponentTree::Traverse(screen, SlokedPath{path}).AsHandle();
                    handle.NewTabber();
                    rsp.Result(true);
                } catch (const SlokedError &err) {
                    rsp.Result(false);
                }
            });
        }

        void HandleNewTextEditor(const std::string &method, const KgrValue &params, Response &rsp) {
            const auto &path = params.AsDictionary()["path"].AsString();
            auto documentId = static_cast<SlokedEditorDocumentSet::DocumentId>(params.AsDictionary()["document"].AsInt());
            root.Lock([&](auto &screen) {
                try {
                    auto &handle = SlokedComponentTree::Traverse(screen, SlokedPath{path}).AsHandle();
                    handle.NewTextPane(std::make_unique<SlokedTextEditor>(this->encoding, this->cursorService(), this->renderService(), documentId));
                    rsp.Result(true);
                } catch (const SlokedError &err) {
                    rsp.Result(false);
                }
            });
        }

        void MultiplexerNewWindow(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsDictionary()["path"].AsString()};
            TextPosition pos {
                static_cast<TextPosition::Line>(params.AsDictionary()["pos"].AsDictionary()["line"].AsInt()),
                static_cast<TextPosition::Column>(params.AsDictionary()["pos"].AsDictionary()["column"].AsInt())
            };
            TextPosition dim {
                static_cast<TextPosition::Line>(params.AsDictionary()["size"].AsDictionary()["line"].AsInt()),
                static_cast<TextPosition::Column>(params.AsDictionary()["size"].AsDictionary()["column"].AsInt())
            };
            root.Lock([&](auto &screen) {
                try {
                    auto &multiplexer = SlokedComponentTree::Traverse(screen, path.GetChild("self")).AsMultiplexer();
                    auto win = multiplexer.NewWindow(pos, dim);
                    if (win) {
                        rsp.Result(path.GetChild(std::to_string(win->GetId())).ToString());
                    } else {
                        rsp.Result({});
                    }
                } catch (const SlokedError &err) {
                    rsp.Result({});
                }
            });
        }

        void MultiplexerGetInfo(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            root.Lock([&](auto &screen) {
                try {
                    auto &multiplexer = SlokedComponentTree::Traverse(screen, path.GetChild("self")).AsMultiplexer();
                    KgrDictionary response {
                        { "windowCount", static_cast<int64_t>(multiplexer.GetWindowCount()) }
                    };
                    if (multiplexer.GetFocus()) {
                        response.Put("focus", path.GetChild(std::to_string(multiplexer.GetFocus()->GetId())).ToString());
                    }
                    rsp.Result(std::move(response));
                } catch (const SlokedError &err) {
                    rsp.Result({});
                }
            });
        }

        void MultiplexerWindowHasFocus(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            SlokedPath multiplexerPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            root.Lock([&](auto &screen) {
                try {
                    auto &multiplexer = SlokedComponentTree::Traverse(screen, multiplexerPath).AsMultiplexer();
                    auto win = multiplexer.GetWindow(winId);
                    if (win) {
                        rsp.Result(win->HasFocus());
                    } else {
                        rsp.Result({});
                    }
                } catch (const SlokedError &err) {
                    rsp.Result({});
                }
            });
        }

        void MultiplexerSetFocus(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            SlokedPath multiplexerPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            root.Lock([&](auto &screen) {
                try {
                    auto &multiplexer = SlokedComponentTree::Traverse(screen, multiplexerPath).AsMultiplexer();
                    auto win = multiplexer.GetWindow(winId);
                    if (win) {
                        win->SetFocus();
                        rsp.Result(true);
                    } else {
                        rsp.Result(false);
                    }
                } catch (const SlokedError &err) {
                    rsp.Result(false);
                }
            });   
        }

        void MultiplexerClose(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            SlokedPath multiplexerPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            root.Lock([&](auto &screen) {
                try {
                    auto &multiplexer = SlokedComponentTree::Traverse(screen, multiplexerPath).AsMultiplexer();
                    auto win = multiplexer.GetWindow(winId);
                    if (win) {
                        win->Close();
                        rsp.Result(true);
                    } else {
                        rsp.Result(false);
                    }
                } catch (const SlokedError &err) {
                    rsp.Result(false);
                }
            }); 
        }

        void MultiplexerMoveWindow(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsDictionary()["path"].AsString()};
            SlokedPath multiplexerPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            TextPosition pos {
                static_cast<TextPosition::Line>(params.AsDictionary()["pos"].AsDictionary()["line"].AsInt()),
                static_cast<TextPosition::Column>(params.AsDictionary()["pos"].AsDictionary()["column"].AsInt())
            };
            root.Lock([&](auto &screen) {
                try {
                    auto &multiplexer = SlokedComponentTree::Traverse(screen, multiplexerPath).AsMultiplexer();
                    auto win = multiplexer.GetWindow(winId);
                    if (win) {
                        win->Move(pos);
                        rsp.Result(true);
                    } else {
                        rsp.Result(false);
                    }
                } catch (const SlokedError &err) {
                    rsp.Result(false);
                }
            });
        }

        void MultiplexerResizeWindow(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsDictionary()["path"].AsString()};
            SlokedPath multiplexerPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            TextPosition pos {
                static_cast<TextPosition::Line>(params.AsDictionary()["size"].AsDictionary()["line"].AsInt()),
                static_cast<TextPosition::Column>(params.AsDictionary()["size"].AsDictionary()["column"].AsInt())
            };
            root.Lock([&](auto &screen) {
                try {
                    auto &multiplexer = SlokedComponentTree::Traverse(screen, multiplexerPath).AsMultiplexer();
                    auto win = multiplexer.GetWindow(winId);
                    if (win) {
                        win->Resize(pos);
                        rsp.Result(true);
                    } else {
                        rsp.Result(false);
                    }
                } catch (const SlokedError &err) {
                    rsp.Result(false);
                }
            });
        }

        void SplitterNewWindow(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsDictionary()["path"].AsString()};
            Splitter::Constraints constraints(params.AsDictionary()["constraints"].AsDictionary()["dim"].AsNumber(),
                params.AsDictionary()["constraints"].AsDictionary()["min"].AsInt(),
                params.AsDictionary()["constraints"].AsDictionary()["max"].AsInt());
            root.Lock([&](auto &screen) {
                try {
                    auto &splitter = SlokedComponentTree::Traverse(screen, path.GetChild("self")).AsSplitter();
                    auto win = splitter.NewWindow(constraints);
                    if (win) {
                        rsp.Result(path.GetChild(std::to_string(win->GetId())).ToString());
                    } else {
                        rsp.Result({});
                    }
                } catch (const SlokedError &err) {
                    rsp.Result({});
                }
            });
        }

        void SplitterInsertWindow(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsDictionary()["path"].AsString()};
            SlokedPath splitterPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            Splitter::Constraints constraints(params.AsDictionary()["constraints"].AsDictionary()["dim"].AsNumber(),
                params.AsDictionary()["constraints"].AsDictionary()["min"].AsInt(),
                params.AsDictionary()["constraints"].AsDictionary()["max"].AsInt());
            root.Lock([&](auto &screen) {
                try {
                    auto &splitter = SlokedComponentTree::Traverse(screen, splitterPath).AsSplitter();
                    auto win = splitter.NewWindow(winId, constraints);
                    if (win) {
                        rsp.Result(path.GetChild(std::to_string(win->GetId())).ToString());
                    } else {
                        rsp.Result({});
                    }
                } catch (const SlokedError &err) {
                    rsp.Result({});
                }
            });
        }

        void SplitterGetInfo(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            root.Lock([&](auto &screen) {
                try {
                    auto &splitter = SlokedComponentTree::Traverse(screen, path.GetChild("self")).AsSplitter();
                    KgrDictionary response {
                        { "windowCount", static_cast<int64_t>(splitter.GetWindowCount()) }
                    };
                    if (splitter.GetFocus()) {
                        response.Put("focus", path.GetChild(std::to_string(splitter.GetFocus()->GetId())).ToString());
                    }
                    rsp.Result(std::move(response));
                } catch (const SlokedError &err) {
                    rsp.Result({});
                }
            });
        }

        void SplitterWindowHasFocus(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            SlokedPath splitterPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            root.Lock([&](auto &screen) {
                try {
                    auto &splitter = SlokedComponentTree::Traverse(screen, splitterPath).AsSplitter();
                    auto win = splitter.GetWindow(winId);
                    if (win) {
                        rsp.Result(win->HasFocus());
                    } else {
                        rsp.Result({});
                    }
                } catch (const SlokedError &err) {
                    rsp.Result({});
                }
            });
        }

        void SplitterSetFocus(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            SlokedPath splitterPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            root.Lock([&](auto &screen) {
                try {
                    auto &splitter = SlokedComponentTree::Traverse(screen, splitterPath).AsSplitter();
                    auto win = splitter.GetWindow(winId);
                    if (win) {
                        win->SetFocus();
                        rsp.Result(true);
                    } else {
                        rsp.Result(false);
                    }
                } catch (const SlokedError &err) {
                    rsp.Result(false);
                }
            });   
        }

        void SplitterClose(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            SlokedPath splitterPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            root.Lock([&](auto &screen) {
                try {
                    auto &splitter = SlokedComponentTree::Traverse(screen, splitterPath).AsSplitter();
                    auto win = splitter.GetWindow(winId);
                    if (win) {
                        win->Close();
                        rsp.Result(true);
                    } else {
                        rsp.Result(false);
                    }
                } catch (const SlokedError &err) {
                    rsp.Result(false);
                }
            }); 
        }

        void SplitterUpdateWindowConstraints(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsDictionary()["path"].AsString()};
            SlokedPath splitterPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            Splitter::Constraints constraints(params.AsDictionary()["constraints"].AsDictionary()["dim"].AsNumber(),
                params.AsDictionary()["constraints"].AsDictionary()["min"].AsInt(),
                params.AsDictionary()["constraints"].AsDictionary()["max"].AsInt());
            root.Lock([&](auto &screen) {
                try {
                    auto &splitter = SlokedComponentTree::Traverse(screen, splitterPath).AsSplitter();
                    auto win = splitter.GetWindow(winId);
                    if (win) {
                        win->UpdateConstraints(constraints);
                        rsp.Result(true);
                    } else {
                        rsp.Result(false);
                    }
                } catch (const SlokedError &err) {
                    rsp.Result(false);
                }
            });
        }

        void SplitterMoveWindow(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsDictionary()["path"].AsString()};
            SlokedPath splitterPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            auto newWinId = static_cast<SlokedComponentWindow::Id>(params.AsDictionary()["position"].AsInt());
            root.Lock([&](auto &screen) {
                try {
                    auto &splitter = SlokedComponentTree::Traverse(screen, splitterPath).AsSplitter();
                    auto win = splitter.GetWindow(winId);
                    if (win) {
                        win->Move(newWinId);
                        rsp.Result(splitterPath.GetChild(std::to_string(newWinId)).ToString());
                    } else {
                        rsp.Result({});
                    }
                } catch (const SlokedError &err) {
                    rsp.Result({});
                }
            });
        }

        void TabberNewWindow(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsDictionary()["path"].AsString()};
            root.Lock([&](auto &screen) {
                try {
                    auto &tabber = SlokedComponentTree::Traverse(screen, path.GetChild("self")).AsTabber();
                    auto win = tabber.NewWindow();
                    rsp.Result(path.GetChild(std::to_string(win->GetId())).ToString());
                } catch (const SlokedError &err) {
                    rsp.Result({});
                }
            });   
        }

        void TabberInsertWindow(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsDictionary()["path"].AsString()};
            SlokedPath tabberPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            root.Lock([&](auto &screen) {
                try {
                    auto &tabber = SlokedComponentTree::Traverse(screen, tabberPath).AsTabber();
                    auto win = tabber.NewWindow(winId);
                    if (win) {
                        rsp.Result(path.GetChild(std::to_string(win->GetId())).ToString());
                    } else {
                        rsp.Result({});
                    }
                } catch (const SlokedError &err) {
                    rsp.Result({});
                }
            });   
        }

        void TabberGetInfo(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            root.Lock([&](auto &screen) {
                try {
                    auto &tabber = SlokedComponentTree::Traverse(screen, path.GetChild("self")).AsTabber();
                    KgrDictionary response {
                        { "windowCount", static_cast<int64_t>(tabber.GetWindowCount()) }
                    };
                    if (tabber.GetFocus()) {
                        response.Put("focus", path.GetChild(std::to_string(tabber.GetFocus()->GetId())).ToString());
                    }
                    rsp.Result(std::move(response));
                } catch (const SlokedError &err) {
                    rsp.Result({});
                }
            });
        }

        void TabberWindowHasFocus(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            SlokedPath tabberPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            root.Lock([&](auto &screen) {
                try {
                    auto &tabber = SlokedComponentTree::Traverse(screen, tabberPath).AsTabber();
                    auto win = tabber.GetWindow(winId);
                    if (win) {
                        rsp.Result(win->HasFocus());
                    } else {
                        rsp.Result({});
                    }
                } catch (const SlokedError &err) {
                    rsp.Result({});
                }
            });
        }

        void TabberSetFocus(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            SlokedPath tabberPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            root.Lock([&](auto &screen) {
                try {
                    auto &tabber = SlokedComponentTree::Traverse(screen, tabberPath).AsTabber();
                    auto win = tabber.GetWindow(winId);
                    if (win) {
                        win->SetFocus();
                        rsp.Result(true);
                    } else {
                        rsp.Result(false);
                    }
                } catch (const SlokedError &err) {
                    rsp.Result(false);
                }
            });   
        }

        void TabberClose(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            SlokedPath tabberPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            root.Lock([&](auto &screen) {
                try {
                    auto &tabber = SlokedComponentTree::Traverse(screen, tabberPath).AsTabber();
                    auto win = tabber.GetWindow(winId);
                    if (win) {
                        win->Close();
                        rsp.Result(true);
                    } else {
                        rsp.Result(false);
                    }
                } catch (const SlokedError &err) {
                    rsp.Result(false);
                }
            }); 
        }

        void TabberMoveWindow(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsDictionary()["path"].AsString()};
            SlokedPath tabberPath = path.GetParent().GetChild("self");
            auto winId = static_cast<SlokedComponentWindow::Id>(std::stoull(path.Components().back()));
            auto newWinId = static_cast<SlokedComponentWindow::Id>(params.AsDictionary()["position"].AsInt());
            root.Lock([&](auto &screen) {
                try {
                    auto &tabber = SlokedComponentTree::Traverse(screen, tabberPath).AsTabber();
                    auto win = tabber.GetWindow(winId);
                    if (win) {
                        win->Move(newWinId);
                        rsp.Result(tabberPath.GetChild(std::to_string(newWinId)).ToString());
                    } else {
                        rsp.Result({});
                    }
                } catch (const SlokedError &err) {
                    rsp.Result({});
                }
            });
        }
    
        SlokedSynchronized<SlokedScreenComponent &> &root;
        const Encoding &encoding;
        KgrServer::Connector cursorService;
        KgrServer::Connector renderService;
    };

    SlokedScreenService::SlokedScreenService(SlokedSynchronized<SlokedScreenComponent &> &root, const Encoding &encoding,
        KgrServer::Connector cursorService, KgrServer::Connector renderService, KgrContextManager<KgrLocalContext> &contextManager)
        : root(root), encoding(encoding), cursorService(std::move(cursorService)), renderService(std::move(renderService)), contextManager(contextManager) {}

    bool SlokedScreenService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedScreenContext>(std::move(pipe), this->root, this->encoding, this->cursorService, this->renderService);
        this->contextManager.Attach(std::move(ctx));
        return true;
    }

    SlokedScreenClient::SlokedScreenClient(std::unique_ptr<KgrPipe> pipe)
        : client(std::move(pipe)), Handle(client), Multiplexer(client), Splitter(client), Tabber(client) {}
    
    SlokedScreenClient::HandleClient::HandleClient(SlokedServiceClient &client)
        : client(client) {}

    bool SlokedScreenClient::HandleClient::NewMultiplexer(const std::string &path) const {
        auto rsp = client.Invoke("handle.newMultiplexer", path);
        auto res = rsp.Get();
        return res.HasResult() && res.GetResult().AsBoolean();
    }

    bool SlokedScreenClient::HandleClient::NewSplitter(const std::string &path, Splitter::Direction direction) const {
        auto rsp = client.Invoke("handle.newSplitter", KgrDictionary {
            { "path", path },
            { "direction", static_cast<int64_t>(direction) }
        });
        auto res = rsp.Get();
        return res.HasResult() && res.GetResult().AsBoolean();
    }

    bool SlokedScreenClient::HandleClient::NewTabber(const std::string &path) const {
        auto rsp = client.Invoke("handle.newTabber", path);
        auto res = rsp.Get();
        return res.HasResult() && res.GetResult().AsBoolean();
    }
    
    bool SlokedScreenClient::HandleClient::NewTextEditor(const std::string &path, SlokedEditorDocumentSet::DocumentId document) const {
        auto rsp = client.Invoke("handle.newTextEditor", KgrDictionary {
            { "path", path },
            { "document", static_cast<int64_t>(document) }
        });
        auto res = rsp.Get();
        return res.HasResult() && res.GetResult().AsBoolean();
    }

    SlokedScreenClient::MultiplexerClient::MultiplexerClient(SlokedServiceClient &client)
        : client(client) {}

    std::optional<std::string> SlokedScreenClient::MultiplexerClient::NewWindow(const std::string &path, const TextPosition &pos, const TextPosition &dim) const {
        auto rsp = client.Invoke("multiplexer.newWindow", KgrDictionary {
            { "path", path },
            {
                "pos", KgrDictionary {
                    { "line", static_cast<int64_t>(pos.line) },
                    { "column", static_cast<int64_t>(pos.column) }
                }
            },
            {
                "size", KgrDictionary {
                    { "line", static_cast<int64_t>(dim.line) },
                    { "column", static_cast<int64_t>(dim.column) }
                }
            }
        });
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::String)) {
            return res.GetResult().AsString();
        } else {
            return {};
        }
    }

    std::size_t SlokedScreenClient::MultiplexerClient::GetWindowCount(const std::string &path) const {
        auto rsp = client.Invoke("multiplexer.getInfo", path);
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::Object)) {
            return static_cast<std::size_t>(res.GetResult().AsDictionary()["windowCount"].AsInt());
        } else {
            return 0;
        }
    }

    std::optional<std::string> SlokedScreenClient::MultiplexerClient::GetFocus(const std::string &path) const {
        auto rsp = client.Invoke("multiplexer.getInfo", path);
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::Object) && res.GetResult().AsDictionary().Has("focus")) {
            return res.GetResult().AsDictionary()["focus"].AsString();
        } else {
            return {};
        }
    }

    std::optional<bool> SlokedScreenClient::MultiplexerClient::WindowHasFocus(const std::string &path) const {
        auto rsp = client.Invoke("multiplexer.windowHasFocus", path);
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::Boolean)) {
            return res.GetResult().AsBoolean();
        } else {
            return {};
        }
    }

    bool SlokedScreenClient::MultiplexerClient::SetFocus(const std::string &path) const {
        auto rsp = client.Invoke("multiplexer.setFocus", path);
        auto res = rsp.Get();
        return res.HasResult() && res.GetResult().AsBoolean();
    }

    bool SlokedScreenClient::MultiplexerClient::Close(const std::string &path) const {
        auto rsp = client.Invoke("multiplexer.close", path);
        auto res = rsp.Get();
        return res.HasResult() && res.GetResult().AsBoolean();
    }

    bool SlokedScreenClient::MultiplexerClient::MoveWindow(const std::string &path, const TextPosition &pos) const {
        auto rsp = client.Invoke("multiplexer.moveWindow", KgrDictionary {
            { "path", path },
            {
                "pos", KgrDictionary {
                    { "line", static_cast<int64_t>(pos.line) },
                    { "column", static_cast<int64_t>(pos.column) }
                }
            }
        });   
        auto res = rsp.Get();
        return res.HasResult() && res.GetResult().AsBoolean();
    }

    bool SlokedScreenClient::MultiplexerClient::ResizeWindow(const std::string &path, const TextPosition &dim) const {
        auto rsp = client.Invoke("multiplexer.resizeWindow", KgrDictionary {
            { "path", path },
            {
                "size", KgrDictionary {
                    { "line", static_cast<int64_t>(dim.line) },
                    { "column", static_cast<int64_t>(dim.column) }
                }
            }
        });   
        auto res = rsp.Get();
        return res.HasResult() && res.GetResult().AsBoolean();
    }
    
    SlokedScreenClient::SplitterClient::SplitterClient(SlokedServiceClient &client)
        : client(client) {}

    std::optional<std::string> SlokedScreenClient::SplitterClient::NewWindow(const std::string &path, const Splitter::Constraints &constraints) const {
        auto rsp = client.Invoke("splitter.newWindow", KgrDictionary {
            { "path", path },
            {
                "constraints", KgrDictionary {
                    { "dim", constraints.GetDimensions() },
                    { "min", static_cast<int64_t>(constraints.GetMinimum()) },
                    { "max", static_cast<int64_t>(constraints.GetMaximum()) }
                }
            }
        });
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::String)) {
            return res.GetResult().AsString();
        } else {
            return {};
        }
    }

    std::optional<std::string> SlokedScreenClient::SplitterClient::NewWindow(const std::string &path, SlokedComponentWindow::Id winId, const Splitter::Constraints &constraints) const {
        auto rsp = client.Invoke("splitter.insertWindow", KgrDictionary {
            { "path", SlokedPath(path).GetChild(std::to_string(winId)).ToString() },
            {
                "constraints", KgrDictionary {
                    { "dim", constraints.GetDimensions() },
                    { "min", static_cast<int64_t>(constraints.GetMinimum()) },
                    { "max", static_cast<int64_t>(constraints.GetMaximum()) }
                }
            }
        });
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::String)) {
            return res.GetResult().AsString();
        } else {
            return {};
        }
    }

    std::size_t SlokedScreenClient::SplitterClient::GetWindowCount(const std::string &path) const {
        auto rsp = client.Invoke("splitter.getInfo", path);
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::Object)) {
            return static_cast<std::size_t>(res.GetResult().AsDictionary()["windowCount"].AsInt());
        } else {
            return 0;
        }
    }

    std::optional<std::string> SlokedScreenClient::SplitterClient::GetFocus(const std::string &path) const {
        auto rsp = client.Invoke("splitter.getInfo", path);
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::Object) && res.GetResult().AsDictionary().Has("focus")) {
            return res.GetResult().AsDictionary()["focus"].AsString();
        } else {
            return {};
        }
    }

    std::optional<bool> SlokedScreenClient::SplitterClient::WindowHasFocus(const std::string &path) const {
        auto rsp = client.Invoke("splitter.windowHasFocus", path);
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::Boolean)) {
            return res.GetResult().AsBoolean();
        } else {
            return {};
        }
    }

    bool SlokedScreenClient::SplitterClient::SetFocus(const std::string &path) const {
        auto rsp = client.Invoke("splitter.setFocus", path);
        auto res = rsp.Get();
        return res.HasResult() && res.GetResult().AsBoolean();
    }

    bool SlokedScreenClient::SplitterClient::Close(const std::string &path) const {
        auto rsp = client.Invoke("splitter.close", path);
        auto res = rsp.Get();
        return res.HasResult() && res.GetResult().AsBoolean();
    }

    std::optional<std::string> SlokedScreenClient::SplitterClient::MoveWindow(const std::string &path, SlokedComponentWindow::Id winId) const {
        auto rsp = client.Invoke("splitter.moveWindow", KgrDictionary {
            { "path", path },
            { "position", static_cast<int64_t>(winId) }
        });
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::String)) {
            return res.GetResult().AsString();
        } else {
            return {};
        }
    }

    bool SlokedScreenClient::SplitterClient::UpdateWindowConstraints(const std::string &path, const Splitter::Constraints &constraints) const {
        auto rsp = client.Invoke("splitter.updateWindowConstraints", KgrDictionary {
            { "path", path },
            {
                "constraints", KgrDictionary {
                    { "dim", constraints.GetDimensions() },
                    { "min", static_cast<int64_t>(constraints.GetMinimum()) },
                    { "max", static_cast<int64_t>(constraints.GetMaximum()) }
                }
            }
        });
        auto res = rsp.Get();
        return res.HasResult() && res.GetResult().AsBoolean();
    }

    SlokedScreenClient::TabberClient::TabberClient(SlokedServiceClient &client)
        : client(client) {}

    std::optional<std::string> SlokedScreenClient::TabberClient::NewWindow(const std::string &path) const {
        auto rsp = client.Invoke("tabber.newWindow", KgrDictionary {
            { "path", path }
        });
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::String)) {
            return res.GetResult().AsString();
        } else {
            return {};
        }
    }

    std::optional<std::string> SlokedScreenClient::TabberClient::NewWindow(const std::string &path, SlokedComponentWindow::Id winId) const {
        auto rsp = client.Invoke("tabber.insertWindow", KgrDictionary {
            { "path", SlokedPath(path).GetChild(std::to_string(winId)).ToString() }
        });
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::String)) {
            return res.GetResult().AsString();
        } else {
            return {};
        }
    }

    std::size_t SlokedScreenClient::TabberClient::GetWindowCount(const std::string &path) const {
        auto rsp = client.Invoke("tabber.getInfo", path);
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::Object)) {
            return static_cast<std::size_t>(res.GetResult().AsDictionary()["windowCount"].AsInt());
        } else {
            return 0;
        }
    }

    std::optional<std::string> SlokedScreenClient::TabberClient::GetFocus(const std::string &path) const {
        auto rsp = client.Invoke("tabber.getInfo", path);
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::Object) && res.GetResult().AsDictionary().Has("focus")) {
            return res.GetResult().AsDictionary()["focus"].AsString();
        } else {
            return {};
        }
    }

    std::optional<bool> SlokedScreenClient::TabberClient::WindowHasFocus(const std::string &path) const {
        auto rsp = client.Invoke("tabber.windowHasFocus", path);
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::Boolean)) {
            return res.GetResult().AsBoolean();
        } else {
            return {};
        }
    }

    bool SlokedScreenClient::TabberClient::SetFocus(const std::string &path) const {
        auto rsp = client.Invoke("tabber.setFocus", path);
        auto res = rsp.Get();
        return res.HasResult() && res.GetResult().AsBoolean();
    }

    bool SlokedScreenClient::TabberClient::Close(const std::string &path) const {
        auto rsp = client.Invoke("tabber.close", path);
        auto res = rsp.Get();
        return res.HasResult() && res.GetResult().AsBoolean();
    }

    std::optional<std::string> SlokedScreenClient::TabberClient::MoveWindow(const std::string &path, SlokedComponentWindow::Id winId) const {
        auto rsp = client.Invoke("tabber.moveWindow", KgrDictionary {
            { "path", path },
            { "position", static_cast<int64_t>(winId) }
        });
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::String)) {
            return res.GetResult().AsString();
        } else {
            return {};
        }
    }
}