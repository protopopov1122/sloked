#include "sloked/filesystem/posix/Reader.h"
#include <memory>

namespace sloked {

    SlokedPosixFileReader::Offset SlokedPosixFileIO::Tell() {
        return ftell(this->file);
    }
    
    SlokedPosixFileReader::SlokedPosixFileReader(FILE *file)
        : SlokedPosixFileIO(file) {}
    
    std::string SlokedPosixFileReader::Read(std::size_t sz) {
        std::unique_ptr<char[]> buffer(new char[sz]);
        std::size_t length = fread(buffer.get(), sizeof(char), sz, this->file);
        return std::string {buffer.get(), length};
    }

    int SlokedPosixFileReader::Read() {
        return fgetc(this->file);
    }

    bool SlokedPosixFileReader::Unread(int c) {
        return ungetc(c, this->file) != EOF;
    }
    
    bool SlokedPosixFileReader::Eof() {
        return static_cast<bool>(feof(this->file));
    }
}