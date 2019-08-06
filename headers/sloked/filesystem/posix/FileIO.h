#ifndef SLOKED_FILESYSTEM_POSIX_FILEIO_H_
#define SLOKED_FILESYSTEM_POSIX_FILEIO_H_

#include "sloked/filesystem/FileIO.h"
#include <cstdio>

namespace sloked {


    class SlokedPosixFileIO : public virtual SlokedFileIO {
     public:
        SlokedPosixFileIO(FILE *);
        virtual ~SlokedPosixFileIO();
        
        SlokedPosixFileIO(const SlokedPosixFileIO &) = delete;
        SlokedPosixFileIO(SlokedPosixFileIO &&);
        SlokedPosixFileIO &operator=(const SlokedPosixFileIO &) = delete;
        SlokedPosixFileIO &operator=(SlokedPosixFileIO &&);

        void Close() override;
        Offset Tell() override;
        bool Seek(Offset, Origin) override;
        bool HasErrors() override;
        void ClearErrors() override;

     protected:
        FILE *file;
    };
}

#endif