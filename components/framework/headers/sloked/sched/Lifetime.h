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

#ifndef SLOKED_SCHED_LIFETIME_H_
#define SLOKED_SCHED_LIFETIME_H_

#include <condition_variable>
#include <memory>
#include <mutex>

#include "sloked/core/Closeable.h"

namespace sloked {

    class SlokedLifetime {
     public:
        class Token {
         public:
            virtual ~Token() = default;
            virtual SlokedLifetime &GetLifetime() const = 0;
        };

        virtual ~SlokedLifetime() = default;
        virtual bool IsActive() const = 0;
        virtual std::unique_ptr<Token> Acquire() = 0;

        static const std::shared_ptr<SlokedLifetime> Global;
    };

    class SlokedStandardLifetime : public SlokedLifetime,
                                   public SlokedCloseable {
     public:
        class StandardToken : public Token {
         public:
            ~StandardToken();
            SlokedLifetime &GetLifetime() const final;

            friend class SlokedStandardLifetime;

         private:
            StandardToken(SlokedStandardLifetime &);

            SlokedStandardLifetime &lifetime;
        };

        friend class StandardToken;

        ~SlokedStandardLifetime();
        void Close() final;
        bool IsActive() const final;
        std::unique_ptr<Token> Acquire() final;

     private:
        mutable std::mutex mtx;
        std::condition_variable cv;
        bool active{true};
        std::size_t tokens{0};
    };
}  // namespace sloked

#endif