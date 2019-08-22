#ifndef SLOKED_CORE_AVL_H_
#define SLOKED_CORE_AVL_H_

#include "sloked/Base.h"
#include <type_traits>
#include <cinttypes>
#include <memory>

namespace sloked {

    template <typename T, typename U = std::unique_ptr<T>>
    class AVLNode {
     protected:
        AVLNode(U begin, U end)
            : begin(std::move(begin)), end(std::move(end)) {
            static_assert(std::is_base_of<AVLNode<T, U>, T>::value);
            static_assert(std::is_convertible_v<decltype(std::declval<const T &>().GetHeight()), std::size_t>);
        }

        virtual ~AVLNode() = default;

        virtual void AvlUpdate() = 0;
        virtual void AvlSwapContent(T &) = 0;

#define HEIGHT(p) (p != nullptr ? p->GetHeight() + 1 : 0)
        bool AvlBalanced() {
            int64_t hB = HEIGHT(this->begin);
            int64_t hE = HEIGHT(this->end);
            int64_t balance = hB - hE;
            return balance > -2 && balance < 2;
        }
        
        void AvlBalance() {
            if (this->begin) {
                this->begin->AvlBalance();
            }
            if (this->end) {
                this->end->AvlBalance();
            }
            int64_t hB = HEIGHT(this->begin);
            int64_t hE = HEIGHT(this->end);
            int64_t balance = hB - hE;
            if (balance < -2 && balance > 2) {
                this->AvlUpdate();
                return;
            } else if (balance == 2) {
                if (HEIGHT(this->begin->begin) > HEIGHT(this->begin->end)) {
                    this->RotateLl();
                } else {
                    this->RotateLr();
                }
            } else if (balance == -2) {
                if (HEIGHT(this->end->end) > HEIGHT(this->end->begin)) {
                    this->RotateRr();
                } else {
                    this->RotateRl();
                }
            } else {
                this->AvlUpdate();
            }
        }
#undef HEIGHT

        U begin;
        U end;

     private:
        void RotateLl() {
            if (this->begin) {
                this->AvlSwapContent(*this->begin);
                U t1 = std::move(this->begin->begin);
                U t23 = std::move(this->begin->end);
                U t4 = std::move(this->end);
                this->end = std::move(this->begin);
                this->begin = std::move(t1);
                this->end->begin = std::move(t23);
                this->end->end = std::move(t4);
                static_cast<AVLNode<T, U> *>(this->end.get())->AvlUpdate();
                this->AvlUpdate();
            }
        }

        void RotateRr() {
            if (this->end) {
                this->AvlSwapContent(*this->end);
                U t1 = std::move(this->begin);
                U t23 = std::move(this->end->begin);
                U t4 = std::move(this->end->end);
                this->begin = std::move(this->end);
                this->begin->begin = std::move(t1);
                this->begin->end = std::move(t23);
                this->end = std::move(t4);
                static_cast<AVLNode<T, U> *>(this->begin.get())->AvlUpdate();
                this->AvlUpdate();
            }
        }
        
        void RotateLr() {
            if (this->begin) {
                this->begin->RotateRr();
                this->RotateLl();
            }
        }

        void RotateRl() {
            if (this->end) {
                this->end->RotateLl();
                this->RotateRr();
            }
        }
    };
}

#endif