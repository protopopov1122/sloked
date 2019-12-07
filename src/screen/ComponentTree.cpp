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

#include "sloked/screen/components/ComponentTree.h"
#include "sloked/screen/components/MultiplexerComponent.h"
#include "sloked/screen/components/SplitterComponent.h"
#include "sloked/screen/components/TabberComponent.h"
#include "sloked/core/Error.h"

namespace sloked {

    SlokedScreenComponent &SlokedComponentTree::Traverse(SlokedScreenComponent &root, const SlokedPath &path) {
        if (!path.Components().empty()) {
            std::string front = path.Components().front();
            if (front == "." ) {
                if (path.Components().size() > 1) {
                    front = path.Components()[1];
                } else {
                    return root;
                }
            } else if (front == "..") {
                throw SlokedError("Component traverse: '..' is unaccessible");
            }
            if (root.GetType() == SlokedScreenComponent::Type::Handle) {
                return SlokedComponentTree::Traverse(root.AsHandle().GetComponent(), path);
            }
            SlokedPath tail = path.Tail(1);
            if (front == "self") {
                return SlokedComponentTree::Traverse(root, tail);
            }
            std::size_t id = std::stoull(front);
            switch (root.GetType()) {                
                case SlokedScreenComponent::Type::Multiplexer: {
                    auto &multiplexer = root.AsMultiplexer();
                    auto win = multiplexer.GetWindow(id);
                    if (win) {
                        return SlokedComponentTree::Traverse(win->GetComponent(), tail);
                    } else {
                        throw SlokedError("Component traverse: unaccessible component \'" + path.ToString() + "\'");
                    }
                }
                
                case SlokedScreenComponent::Type::Splitter: {
                    auto &splitter = root.AsSplitter();
                    auto win = splitter.GetWindow(id);
                    if (win) {
                        return SlokedComponentTree::Traverse(win->GetComponent(), tail);
                    } else {
                        throw SlokedError("Component traverse: unaccessible component \'" + path.ToString() + "\'");
                    }
                }

                case SlokedScreenComponent::Type::Tabber: {
                    auto &tabber = root.AsTabber();
                    auto win = tabber.GetWindow(id);
                    if (win) {
                        return SlokedComponentTree::Traverse(win->GetComponent(), tail);
                    } else {
                        throw SlokedError("Component traverse: unaccessible component \'" + path.ToString() + "\'");
                    }
                }

                case SlokedScreenComponent::Type::TextPane:
                    throw SlokedError("Component traverse: text pane is not traversable");
                
                default:
                    throw SlokedError("Component traverse: component type is not traversable");
            }
        }
        return root;
    }
}