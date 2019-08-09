#ifndef SLOKED_NAMESPACE_PATH_H_
#define SLOKED_NAMESPACE_PATH_H_

#include "sloked/Base.h"
#include <string>
#include <vector>
#include <functional>

namespace sloked {

    class SlokedPath {
     public:
        using Visitor = std::function<void(const std::string &)>;

        SlokedPath(std::string_view);
        SlokedPath(const SlokedPath &) = default;
        SlokedPath(SlokedPath &&) = default;

        SlokedPath &operator=(const SlokedPath &) = default;
        SlokedPath &operator=(SlokedPath &&) = default;

        bool IsAbsolute() const;
        const std::string &ToString() const;
        SlokedPath RelativeTo(const SlokedPath &) const;
        void Iterate(Visitor) const;
        SlokedPath GetParent() const;
        SlokedPath GetChild(std::string_view) const;
        bool IsChildOrSibling(const SlokedPath &) const;

        static constexpr auto Separator = '/';
        static constexpr auto CurrentDir = ".";
        static constexpr auto ParentDir = "..";

     private:
        SlokedPath() = default;
        SlokedPath &Normalize();

        std::vector<std::string> path;
        std::string literal;
    };
}

#endif