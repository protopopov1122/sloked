#ifndef SLOKED_NAMESPACE_FILESYSTEM_H_
#define SLOKED_NAMESPACE_FILESYSTEM_H_

#include "sloked/namespace/Object.h"
#include "sloked/filesystem/File.h"

namespace sloked {

   class SlokedFilesystemAdapter {
   public:
      virtual ~SlokedFilesystemAdapter() = default;
      virtual const SlokedPath &GetRoot() const = 0;
      virtual std::unique_ptr<SlokedFile> NewFile(const SlokedPath &) const = 0;
   };

   class SlokedFilesystemNamespace : public SlokedNamespace {
   public:
      SlokedFilesystemNamespace(std::unique_ptr<SlokedFilesystemAdapter>);
      std::unique_ptr<SlokedNamespaceObject> GetObject(const SlokedPath &) override;
      bool HasObject(const SlokedPath &) const override;
      void Iterate(const SlokedPath &, Visitor) const override;
      std::unique_ptr<SlokedNamespaceObjectHandle> GetHandle(const SlokedPath &) override;

   private:
      std::unique_ptr<SlokedFilesystemAdapter> filesystem;
   };
}

#endif