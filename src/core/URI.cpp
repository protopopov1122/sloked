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

#include "sloked/core/URI.h"
#include "sloked/core/Locale.h"
#include "sloked/core/Error.h"
#include <sstream>
#include <iomanip>
#include <regex>

using namespace std::literals::string_literals;

namespace sloked {

    static constexpr char DigitToHex(uint8_t digit) {
        if (digit < 10) {
            return '0' + digit;
        } else switch (digit) {
            case 10:
                return 'A';
            
            case 11:
                return 'B';

            case 12:
                return 'C';

            case 13:
                return 'D';

            case 14:
                return 'E';

            case 15:
                return 'F';

            default:
                return '.';
        }
    }

    static const std::string ComponentNoEscape = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789"
                                        "-_.!~*'()";

    static const std::string UriNoEscape = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                           "abcdefghijklmnopqrstuvwxyz"
                                           "0123456789"
                                           ";,/?:@&=+$-_.!~*'()#";

    static std::string encodeComponent(std::string_view input, std::string_view notEscaped = ComponentNoEscape) {
        std::stringstream ss;
        SlokedLocale::SystemEncoding().IterateCodepoints(input, [&](auto position, auto length, auto chr) {
            auto content = input.substr(position, length);
            if (notEscaped.find_first_of(chr) != notEscaped.npos) {
                ss << content;
            } else {
                for (auto b : content) {
                    ss << '%' << DigitToHex((b & 0xf0) >> 4) << DigitToHex(b & 0xf);
                }
            }
            return true;
        });
        return ss.str();
    }

    SlokedUri::Authority::Authority(std::string host, std::optional<Port> port)
        : host(std::move(host)), port(std::move(port)) {}

    SlokedUri::Authority::Authority(std::string userinfo, std::string host, std::optional<Port> port)
        : userinfo(std::move(userinfo)), host(std::move(host)), port(std::move(port)) {}

    const std::optional<std::string> &SlokedUri::Authority::GetUserinfo() const {
        return this->userinfo;
    }

    SlokedUri::Authority &SlokedUri::Authority::SetUserinfo(std::optional<std::string> userinfo) {
        this->userinfo = std::move(userinfo);
        return *this;
    }

    const std::string &SlokedUri::Authority::GetHost() const {
        return this->host;
    }

    SlokedUri::Authority &SlokedUri::Authority::SetHost(std::string host) {
        this->host = std::move(host);
        return *this;
    }

    const std::optional<SlokedUri::Authority::Port> &SlokedUri::Authority::GetPort() const {
        return this->port;
    }

    SlokedUri::Authority &SlokedUri::Authority::SetPort(std::optional<Port> port) {
        this->port = std::move(port);
        return *this;
    }

    std::ostream &operator<<(std::ostream &os, const SlokedUri::Authority &auth) {
        if (auth.userinfo.has_value()) {
            os << encodeComponent(auth.userinfo.value()) << '@';
        }
        os << encodeComponent(auth.host);
        if (auth.port.has_value()) {
            os << ':' << auth.port.value();
        }
        return os;
    }
    
    SlokedUri::Query::Query(std::map<std::string, std::string> content)
        : prms(std::move(content)) {}

    SlokedUri::Query &SlokedUri::Query::Put(std::string key, std::string value) {
        this->prms.emplace(std::move(key), std::move(value));
        return *this;
    }

    bool SlokedUri::Query::Has(const std::string &key) const {
        return this->prms.count(key) != 0;
    }

    const std::string &SlokedUri::Query::Get(const std::string &key) const {
        if (this->Has(key)) {
            return this->prms.at(key);
        } else {
            throw SlokedError("URI: parameter not found");
        }
    }

    bool SlokedUri::Query::Empty() const {
        return this->prms.empty();
    }

    std::ostream &operator<<(std::ostream &os, const SlokedUri::Query &query) {
        for (auto it = query.prms.begin(); it != query.prms.end();) {
            os << encodeComponent(it->first) << '=' << encodeComponent(it->second);
            if (++it != query.prms.end()) {
                os << '&';
            }
        }
        return os;
    }

    SlokedUri::SlokedUri(std::string scheme, std::string path)
        : scheme(std::move(scheme)), path(std::move(path)) {}

    SlokedUri::SlokedUri(std::string scheme, Authority auth, std::string path, std::optional<Query> query, std::optional<std::string> fragment)
        : scheme(std::move(scheme)), authority(std::move(auth)), path(std::move(path)), query(std::move(query)), fragment(std::move(fragment)) {}

    const std::string &SlokedUri::GetScheme() const {
        return this->scheme;
    }

    SlokedUri &SlokedUri::SetScheme(std::string scheme) {
        this->scheme = std::move(scheme);
        return*this;
    }

    const std::optional<SlokedUri::Authority> SlokedUri::GetAuthority() const {
        return this->authority;
    }

    SlokedUri &SlokedUri::SetAuthority(Authority auth) {
        this->authority = std::move(auth);
        return *this;
    }

    const std::string &SlokedUri::GetPath() const {
        return this->path;
    }

    SlokedUri &SlokedUri::SetPath(std::string path) {
        this->path = std::move(path);
        return *this;
    }

    const std::optional<SlokedUri::Query> &SlokedUri::GetQuery() const {
        return this->query;
    }

    SlokedUri &SlokedUri::SetQuery(Query query) {
        this->query = std::move(query);
        return *this;
    }

    const std::optional<std::string> &SlokedUri::GetFragment() const {
        return this->fragment;
    }

    SlokedUri &SlokedUri::SetFragment(std::optional<std::string> fragment) {
        this->fragment = std::move(fragment);
        return *this;
    }

    std::string SlokedUri::ToString() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }

    std::ostream &operator<<(std::ostream &os, const SlokedUri &uri) {
        os << encodeComponent(uri.scheme) << ':';
        if (uri.authority.has_value()) {
            os << "//" << uri.authority.value();
        } else if (uri.path.front() == '/') {
            os << "//";
        }
        if (!uri.path.empty() && uri.path.front() != '/' && uri.authority.has_value()) {
            os << '/';
        }
        os << encodeComponent(uri.path, UriNoEscape);
        if (uri.query.has_value() && !uri.query.value().Empty()) {
            os << '?' << uri.query.value();
        }
        if (uri.fragment.has_value()) {
            os << '#' << encodeComponent(uri.fragment.value());
        }
        return os;
    }

    SlokedUri SlokedUri::Parse(const std::string &input) {
        static const std::regex uriRegex(R"(^(([^:\/?#]+):)?(\/\/([^\/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)");
        static const std::regex authorityRegex(R"(^(([^\/?#]*)@)?(([^\/?#]*?)(:(\d+))?)$)");
        static const std::regex queryRegex(R"((([^#=]+)=([^#&]+)(&|#|)))");
        std::smatch match, authMatch;
        if (std::regex_match(input, match, uriRegex)) {
            std::string schema = match.str(2);
            if (schema.empty()) {
                throw SlokedError("URI: Schema can't be empty");
            }
            SlokedUri uri{std::move(schema)};
            if (!match.str(3).empty()) {
                const auto &auth = match.str(4);
                if (std::regex_match(auth, authMatch, authorityRegex)) {
                    Authority authority{authMatch.str(4)};
                    if (!authMatch.str(1).empty()) {
                        authority.SetUserinfo(authMatch.str(2));
                    }
                    if (!authMatch.str(5).empty()) {
                        authority.SetPort(std::stoull(authMatch.str(6)));
                    }
                    uri.SetAuthority(std::move(authority));
                } else {
                    throw SlokedError("URI: Authority failed");
                }
            }
            uri.SetPath(match.str(5));
            if (!match.str(6).empty()) {
                Query query;
                auto rawQuery = match.str(7);
                for (auto i = std::sregex_iterator(rawQuery.begin(), rawQuery.end(), queryRegex); i != std::sregex_iterator(); ++i) {
                    std::smatch queryMatch = *i;
                    query.Put(queryMatch.str(2), queryMatch.str(3));
                }
                uri.SetQuery(std::move(query));
            }
            if (!match.str(8).empty()) {
                uri.SetFragment(match.str(9));
            }
            return uri;
        } else {
            throw SlokedError("URI: Parsing failed");
        }
    }

    std::string SlokedUri::encodeURI(std::string_view value) {
        return encodeComponent(value, UriNoEscape);
    }

    std::string SlokedUri::encodeURIComponent(std::string_view value) {
        return encodeComponent(value, ComponentNoEscape);
    }
}