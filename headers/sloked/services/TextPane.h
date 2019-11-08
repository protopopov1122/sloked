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

#ifndef SLOKED_SERVICES_TEXTPANE_H_
#define SLOKED_SERVICES_TEXTPANE_H_

#include "sloked/services/Service.h"
#include "sloked/kgr/Service.h"
#include "sloked/kgr/Server.h"
#include "sloked/core/Synchronized.h"
#include "sloked/screen/Component.h"
#include "sloked/core/Encoding.h"
#include "sloked/screen/widgets/TextPane.h"

namespace sloked {

    class SlokedTextPaneService : public KgrService {
     public:
        SlokedTextPaneService(SlokedSynchronized<SlokedScreenComponent &> &, const Encoding &, KgrContextManager<KgrLocalContext> &);
        bool Attach(std::unique_ptr<KgrPipe>) override;
    
     private:
        SlokedSynchronized<SlokedScreenComponent &> &root;
        const Encoding &encoding;
        KgrContextManager<KgrLocalContext> &contextManager;
    };
    
    class SlokedTextPaneClient {
     public:
        class Render : public SlokedTextPane {
         public:
            virtual void Reset() = 0;
            virtual void Flush() = 0;
        };

        SlokedTextPaneClient(std::unique_ptr<KgrPipe>);
        ~SlokedTextPaneClient();
        bool Connect(const std::string &, bool, const std::vector<std::pair<SlokedControlKey, bool>> &);
        Render &GetRender();
        void Close();
        std::vector<SlokedKeyboardInput> GetInput();

     private:
        class SlokedTextPaneRender;

        SlokedServiceClient client;
        std::unique_ptr<Render> render;
    };
}

#endif