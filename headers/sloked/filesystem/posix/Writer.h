#ifndef SLOKED_FILESYSTEM_POSIX_WRITER_H_
#define SLOKED_FILESYSTEM_POSIX_WRITER_H_

#include "sloked/filesystem/posix/FileIO.h"

namespace sloked {

    class SlokedPosixFileWriter : public SlokedPosixFileIO, public SlokedIOWriter {
     public:
        SlokedPosixFileWriter(FILE *);
        SlokedPosixFileWriter(SlokedPosixFileWriter &&) = default;
        SlokedPosixFileWriter &operator=(SlokedPosixFileWriter &&) = default;

        std::size_t Write(std::string_view) override;
        bool Write(Char) override;
        bool Flush() override;
    };
}

#endif