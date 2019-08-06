#include "sloked/filesystem/posix/FileIO.h"

namespace sloked {

    SlokedPosixFileIO::SlokedPosixFileIO(FILE *file)
        : file(file) {}

    SlokedPosixFileIO::~SlokedPosixFileIO() {
        this->Close();
    }

    SlokedPosixFileIO::SlokedPosixFileIO(SlokedPosixFileIO &&io)
        : file(io.file) {
        io.file = nullptr;
    }

    SlokedPosixFileIO &SlokedPosixFileIO::operator=(SlokedPosixFileIO &&io) {
        this->Close();
        this->file = io.file;
        io.file = nullptr;
        return *this;
    }

    void SlokedPosixFileIO::Close() {
        if (this->file) {
            fclose(this->file);
            this->file = nullptr;
        }
    }

    bool SlokedPosixFileIO::HasErrors() {
        return ferror(this->file) != 0;
    }

    bool SlokedPosixFileIO::Seek(Offset pos, Origin origin) {
        static int origins[] = {
            SEEK_SET,
            SEEK_CUR,
            SEEK_END
        };
        return fseek(this->file, pos, origins[static_cast<std::size_t>(origin)]);
    }

    void SlokedPosixFileIO::ClearErrors() {
        clearerr(this->file);
    }
}
