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
#include <type_traits>

namespace sloked {

   template <typename T>
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

      template <typename T>
      T Visit(JsonASTVisitor<T> &) const;

      friend std::ostream &operator<<(std::ostream &, const JsonASTNode &);

    protected:
      virtual void VisitNode(JsonASTVisitor<void> &) const = 0;

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

    protected:
      void VisitNode(JsonASTVisitor<void> &) const override;
   
    private:
      DataType type;
      std::variant<int64_t, double, bool, std::string> value;
   };

   class JsonArrayNode : public JsonASTNode {
    public:
      JsonArrayNode(std::vector<std::shared_ptr<JsonASTNode>> &&, const JsonSourcePosition &);
      std::size_t Length() const;
      JsonASTNode &At(std::size_t) const;

    protected:
      void VisitNode(JsonASTVisitor<void> &) const override;

    private:
      std::vector<std::shared_ptr<JsonASTNode>> elements;
   };

   class JsonObjectNode : public JsonASTNode {
    public:
      JsonObjectNode(std::map<std::string, std::unique_ptr<JsonASTNode>> &&, const JsonSourcePosition &);
      bool Has(const std::string &) const;
      JsonASTNode &Get(const std::string &) const;
      const std::set<std::string> &Keys() const;

    protected:
      void VisitNode(JsonASTVisitor<void> &) const override;

    private:
      std::map<std::string, std::unique_ptr<JsonASTNode>> members;
      std::set<std::string> keys;
   };

   template <typename T>
   class JsonASTVisitor {
    public:
      virtual ~JsonASTVisitor() = default;
      virtual T Visit(const JsonConstantNode &) = 0;
      virtual T Visit(const JsonArrayNode &) = 0;
      virtual T Visit(const JsonObjectNode &) = 0;
   };

   template <typename T>
   class JsonASTProxyVisitor : public JsonASTVisitor<void> {
    public:
      JsonASTProxyVisitor(JsonASTVisitor<T> &visitor)
         : visitor(visitor) {}

      void Visit(const JsonConstantNode &node) override {
         this->value = this->visitor.Visit(node);
      }

      void Visit(const JsonArrayNode &node) override {
         this->value = this->visitor.Visit(node);
      }

      void Visit(const JsonObjectNode &node) override {
         this->value = this->visitor.Visit(node);
      }

      T &GetValue() {
         return this->value.value();
      }

    private:
      JsonASTVisitor<T> &visitor;
      std::optional<T> value;
   };

   template <typename T>
   T JsonASTNode::Visit(JsonASTVisitor<T> &visitor) const {
      if constexpr (std::is_void_v<T>) {
         this->VisitNode(visitor);
      } else {
         JsonASTProxyVisitor<T> proxy(visitor);
         this->VisitNode(proxy);
         return proxy.GetValue();
      }
   }
}

#endif