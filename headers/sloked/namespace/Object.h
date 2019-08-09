#ifndef SLOKED_NAMESPACE_OBJECT_H_
#define SLOKED_NAMESPACE_OBJECT_H_

#include "sloked/Base.h"
#include "sloked/core/IO.h"
#include "sloked/namespace/Path.h"
#include <memory>
#include <string>
#include <functional>

namespace sloked {

    class SlokedNamespaceDocument;

    class SlokedNamespaceObject {
     public:
        enum class Type {
            Document,
            Directory,
            None
        };

        virtual ~SlokedNamespaceObject() = default;
        virtual Type GetType() const = 0;
        virtual const SlokedPath &GetPath() const = 0;
        virtual SlokedNamespaceDocument *AsDocument();
    };

    class SlokedNamespaceDocument : public SlokedNamespaceObject {
     public:
        virtual std::unique_ptr<SlokedIOReader> Reader() const = 0;
        virtual std::unique_ptr<SlokedIOWriter> Writer() = 0;
        virtual std::unique_ptr<SlokedIOView> View() const = 0;
    };

    class SlokedNamespace {
     public:
        using Visitor = std::function<void(const std::string &, SlokedNamespaceObject::Type)>;

        virtual ~SlokedNamespace() = default;
        virtual std::unique_ptr<SlokedNamespaceObject> GetObject(const SlokedPath &) const = 0;
        virtual bool HasObject(const SlokedPath &) const = 0;
        virtual void Iterate(const SlokedPath &, Visitor) const = 0;

        virtual void MakeDir(const SlokedPath &) = 0;
        virtual void Delete(const SlokedPath &) = 0;
        virtual void Rename(const SlokedPath &, const SlokedPath &) = 0;
    };
}

#endif