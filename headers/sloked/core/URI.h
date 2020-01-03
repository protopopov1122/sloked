/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

#ifndef SLOKED_CORE_URI_H_
#define SLOKED_CORE_URI_H_

#include "sloked/Base.h"
#include <string>
#include <optional>
#include <iosfwd>
#include <map>

namespace sloked {

    class SlokedUri {
     public:
        class Authority {
         public:
            using Port = uint16_t;
            Authority(std::string, std::optional<Port> = {});
            Authority(std::string, std::string, std::optional<Port> = {});
            Authority(const Authority &) = default;
            Authority(Authority &&) = default;

            Authority &operator=(const Authority &) = default;
            Authority &operator=(Authority &&) = default;

            const std::optional<std::string> &GetUserinfo() const;
            Authority &SetUserinfo(std::optional<std::string>);
            const std::string &GetHost() const;
            Authority &SetHost(std::string);
            const std::optional<Port> &GetPort() const;
            Authority &SetPort(std::optional<Port>);


            friend std::ostream &operator<<(std::ostream &, const Authority &);

         private:
            std::optional<std::string> userinfo;
            std::string host;
            std::optional<Port> port;
        };

        class Query {
         public:
            Query() = default;
            Query(std::map<std::string, std::string>);
            Query &Put(std::string, std::string);
            bool Has(const std::string &) const;
            const std::string &Get(const std::string &) const;
            bool Empty() const;

            friend std::ostream &operator<<(std::ostream &, const Query &);
        
         private:
            std::map<std::string, std::string> prms;
        };

        explicit SlokedUri(std::string, std::string = "");
        explicit SlokedUri(std::string, Authority, std::string = "", std::optional<Query> = {}, std::optional<std::string> = {});
        SlokedUri(const SlokedUri &) = default;
        SlokedUri(SlokedUri &&) = default;

        SlokedUri &operator=(const SlokedUri &) = default;
        SlokedUri &operator=(SlokedUri &&) = default;

        const std::string &GetScheme() const;
        SlokedUri &SetScheme(std::string);
        const std::optional<Authority> GetAuthority() const;
        SlokedUri &SetAuthority(Authority);
        const std::string &GetPath() const;
        SlokedUri &SetPath(std::string);
        const std::optional<Query> &GetQuery() const;
        SlokedUri &SetQuery(Query);
        const std::optional<std::string> &GetFragment() const;
        SlokedUri &SetFragment(std::optional<std::string>);

        std::string ToString() const;

        friend std::ostream &operator<<(std::ostream &, const SlokedUri &);
        static SlokedUri Parse(const std::string &);
        static std::string encodeURI(std::string_view);
        static std::string encodeURIComponent(std::string_view);

     private:
        std::string scheme;
        std::optional<Authority> authority;
        std::string path;
        std::optional<Query> query;
        std::optional<std::string> fragment;
    };
}

#endif