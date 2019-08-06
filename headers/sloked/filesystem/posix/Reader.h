#ifndef SLOKED_FILESYSTEM_POSIX_READER_H_
#define SLOKED_FILESYSTEM_POSIX_READER_H_

#include "sloked/filesystem/posix/FileIO.h"

namespace sloked {

    class SlokedPosixFileReader : public SlokedPosixFileIO, public SlokedFileReader {
     public:
        SlokedPosixFileReader(FILE *);
        SlokedPosixFileReader(SlokedPosixFileReader &&) = default;
        SlokedPosixFileReader &operator=(SlokedPosixFileReader &&) = default;

        std::string Read(std::size_t) override;
        int Read() override;
        bool Unread(int) override;
        bool Eof() override;
    };
}

#endif