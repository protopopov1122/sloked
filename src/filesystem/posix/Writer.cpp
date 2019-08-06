#include "sloked/filesystem/posix/Writer.h"

namespace sloked {

    SlokedPosixFileWriter::SlokedPosixFileWriter(FILE *file)
        : SlokedPosixFileIO(file) {}
    
    std::size_t SlokedPosixFileWriter::Write(std::string_view str) {
        return fwrite(str.data(), sizeof(std::string_view::value_type), str.size(), this->file);
    }
    
    bool SlokedPosixFileWriter::Write(Char chr) {
        return fputc(chr, this->file) != EOF;
    }

    bool SlokedPosixFileWriter::Flush() {
        return fflush(this->file) != EOF;
    }
}