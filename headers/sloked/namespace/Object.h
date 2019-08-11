#ifndef SLOKED_NAMESPACE_OBJECT_H_
#define SLOKED_NAMESPACE_OBJECT_H_

#include "sloked/Base.h"
#include "sloked/core/IO.h"
#include "sloked/core/Permission.h"
#include "sloked/namespace/Path.h"
#include <memory>
#include <string>
#include <functional>

namespace sloked {

    class SlokedNamespaceFile;
    class SlokedNamespaceDirectory;
    class SlokedNamespace;

    class SlokedNamespaceObject {
     public:
        enum class Type {
            File,
            Directory,
            None
        };

        virtual ~SlokedNamespaceObject() = default;
        virtual Type GetType() const = 0;
        virtual const SlokedPath &GetPath() const = 0;
        virtual SlokedNamespaceFile *AsFile();
        virtual SlokedNamespaceDirectory *AsDirectory();
    };

    class SlokedNamespaceFile : public SlokedNamespaceObject {
     public:
        virtual std::unique_ptr<SlokedIOReader> Reader() const = 0;
        virtual std::unique_ptr<SlokedIOWriter> Writer() = 0;
        virtual std::unique_ptr<SlokedIOView> View() const = 0;
    };

    enum class SlokedNamespacePermission {
        Read = 0,
        Write
    };

    class SlokedNamespaceObjectHandle : public SlokedPermissionAuthority<SlokedNamespacePermission> {
     public:
        virtual ~SlokedNamespaceObjectHandle() = default;
        virtual bool Exists() const = 0;
        virtual void MakeDir() = 0;
        virtual void MakeFile() = 0;
        virtual void Delete() = 0;
        virtual void Rename(const SlokedPath &) = 0;
    };

    class SlokedNamespace {
     public:
        using Visitor = std::function<void(const std::string &, SlokedNamespaceObject::Type)>;

        virtual ~SlokedNamespace() = default;
        virtual std::unique_ptr<SlokedNamespaceObject> GetObject(const SlokedPath &) = 0;
        virtual bool HasObject(const SlokedPath &) const = 0;
        virtual void Iterate(const SlokedPath &, Visitor) const = 0;
        virtual std::unique_ptr<SlokedNamespaceObjectHandle> GetHandle(const SlokedPath &) = 0;
    };

    class SlokedNamespaceDirectory : public SlokedNamespace, public SlokedNamespaceObject {};
}

#endif