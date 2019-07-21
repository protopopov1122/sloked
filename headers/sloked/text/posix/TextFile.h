#ifndef SLOKED_TEXT_POSIX_TEXTFILE_H_
#define SLOKED_TEXT_POSIX_TEXTFILE_H_

#include "sloked/Base.h"
#include "sloked/text/TextRegion.h"
#include <iosfwd>
#include <variant>

namespace sloked {

    class PosixTextFile : public TextBlockImpl<PosixTextFile> {
     public:
        PosixTextFile(int);
        virtual ~PosixTextFile();
        PosixTextFile(const PosixTextFile &) = delete;
        PosixTextFile(PosixTextFile &&) = default;

        PosixTextFile &operator=(const PosixTextFile &) = delete;
        PosixTextFile &operator=(PosixTextFile &&) = default;
    
        std::size_t GetLastLine() const override;
        std::size_t GetTotalLength() const override;
        const std::string_view GetLine(std::size_t) const override;
        bool Empty() const override;
        
        void SetLine(std::size_t, const std::string &) override;
        void EraseLine(std::size_t) override;
        void InsertLine(std::size_t, const std::string &) override;
        void Optimize() override;

        std::size_t GetHeight() const {
            return this->content->GetHeight();
        }

        friend std::ostream &operator<<(std::ostream &, const PosixTextFile &);
        
     private:
        void Scan();

        int fd;
        void *data;
        std::size_t length;
        std::unique_ptr<TextRegion> content;
    };
}

#endif