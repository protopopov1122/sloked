#ifndef SLOKED_NAMESPACE_OBJECT_H_
#define SLOKED_NAMESPACE_OBJECT_H_

#include "sloked/Base.h"
#include "sloked/core/IO.h"
#include "sloked/namespace/Path.h"
#include <memory>
#include <string>
#include <functional>

namespace sloked {

    class SlokedNamespace;
    class SlokedNSDocument;

    class SlokedNamespaceObject {
     public:
        enum class Type {
            Document,
            Namespace,
            Unknown
        };

        virtual ~SlokedNamespaceObject() = default;

        virtual Type GetType() const = 0;
        virtual SlokedPath GetPath() const = 0;
        virtual SlokedNSDocument *AsDocument();
        virtual SlokedNamespace *AsNamespace();
    };

    class SlokedNamespace : public virtual SlokedNamespaceObject {
     public:
        using Visitor = std::function<void(const std::string &)>;
        
        virtual std::unique_ptr<SlokedNamespaceObject> GetObject(const std::string &) = 0;
        virtual bool HasObject(const std::string &) = 0;
        virtual void Iterate(Visitor) = 0;
        virtual std::unique_ptr<SlokedNamespaceObject> Find(const SlokedPath &) = 0;
        virtual std::unique_ptr<SlokedNamespace> MakeNamespace(const std::string &) = 0;
        virtual void Delete(const std::string &) = 0;
        virtual void Rename(const std::string &, const SlokedPath &) = 0;
    };

    class SlokedNSDocument : public virtual SlokedNamespaceObject {
     public:
        virtual std::unique_ptr<SlokedIOReader> Reader() const = 0;
        virtual std::unique_ptr<SlokedIOWriter> Writer() const = 0;
        virtual std::unique_ptr<SlokedIOView> View() const = 0;
    };
}

#endif