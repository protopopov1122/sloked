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

#ifndef SLOKED_JSON_AST_H_
#define SLOKED_JSON_AST_H_

#include "sloked/json/Position.h"
#include <string>
#include <cinttypes>
#include <variant>
#include <vector>
#include <memory>
#include <map>
#include <set>

namespace sloked {

   class JsonASTVisitor;

    class JsonASTNode {
     public:
        enum class Type {
            Object,
            Array,
            Constant
        };

        JsonASTNode(Type, const JsonSourcePosition &);
        virtual ~JsonASTNode() = default;
        Type GetType() const;
        const JsonSourcePosition &GetPosition() const;
        
        virtual void Visit(JsonASTVisitor &) const = 0;

        friend std::ostream &operator<<(std::ostream &, const JsonASTNode &);

     private:
        Type type;
        JsonSourcePosition position;
    };

    class JsonConstantNode : public JsonASTNode {
     public:
        enum class DataType {
            Integer,
            Number,
            Boolean,
            String,
            Null
        };

        JsonConstantNode(int64_t, const JsonSourcePosition &);
        JsonConstantNode(double, const JsonSourcePosition &);
        JsonConstantNode(bool, const JsonSourcePosition &);
        JsonConstantNode(std::string_view, const JsonSourcePosition &);
        JsonConstantNode(const JsonSourcePosition &);

        DataType GetConstantType() const;
        int64_t AsInteger(int64_t = 0) const;
        double AsNumber(double = 0.0) const;
        bool AsBoolean(bool = false) const;
        const std::string &AsString(const std::string & = "") const;

        void Visit(JsonASTVisitor &) const override;
    
     private:
        DataType type;
        std::variant<int64_t, double, bool, std::string> value;
    };

    class JsonArrayNode : public JsonASTNode {
     public:
        JsonArrayNode(std::vector<std::shared_ptr<JsonASTNode>> &&, const JsonSourcePosition &);
        std::size_t Length() const;
        JsonASTNode &At(std::size_t) const;

        void Visit(JsonASTVisitor &) const override;

     private:
        std::vector<std::shared_ptr<JsonASTNode>> elements;
    };

    class JsonObjectNode : public JsonASTNode {
     public:
        JsonObjectNode(std::map<std::string, std::unique_ptr<JsonASTNode>> &&, const JsonSourcePosition &);
        bool Has(const std::string &) const;
        JsonASTNode &Get(const std::string &) const;
        const std::set<std::string> &Keys() const;

        void Visit(JsonASTVisitor &) const override;

     private:
        std::map<std::string, std::unique_ptr<JsonASTNode>> members;
        std::set<std::string> keys;
    };

   class JsonASTVisitor {
    public:
      virtual ~JsonASTVisitor() = default;
      virtual void Visit(const JsonConstantNode &) = 0;
      virtual void Visit(const JsonArrayNode &) = 0;
      virtual void Visit(const JsonObjectNode &) = 0;
   };
}

#endif