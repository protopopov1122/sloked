#ifndef SLOKED_SCREEN_SPLITTER_H_
#define SLOKED_SCREEN_SPLITTER_H_

#include "sloked/Base.h"

namespace sloked {

    class Splitter {
     public:
        enum class Direction {
            Vertical,
            Horizontal
        };


        class Constraints {
        public:
            Constraints(float, unsigned int = 0, unsigned int = 0);

            float GetDimensions() const;
            unsigned int GetMinimum() const;
            unsigned int GetMaximum() const;

            unsigned int Calc(unsigned int) const;

        private:
            float dim;
            unsigned int min;
            unsigned int max;
        };
    };
}

#endif