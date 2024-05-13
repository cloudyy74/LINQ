#pragma once

#include <utility>
#include <vector>

namespace linq {
    namespace impl {
        template<typename T, typename Iter>
        class range_enumerator;
        template<typename T>
        class take_enumerator;
        template<typename T>
        class drop_enumerator;
        template<typename T, typename U, typename F>
        class select_enumerator;
        template<typename T, typename F>
        class until_enumerator;
        template<typename T, typename F>
        class where_enumerator;
        
        template<typename T>
        class enumerator {
        public:
            virtual const T& operator*() = 0;
            virtual enumerator& operator++() = 0;
            virtual operator bool() = 0;

            auto take(int amount) {
                return take_enumerator<T>(*this, amount);
            }

            auto drop(int amount) {
                return drop_enumerator<T>(*this, amount);
            }

            template<typename U = T, typename F>
            auto select(F func) {
                return select_enumerator<U, T, F>(*this, std::move(func));
            }

            template<typename F>
            auto until(F func) {
                return until_enumerator<T, F>(*this, std::move(func));
            }

            template<typename F>
            auto where(F func) {
                return where_enumerator<T, F>(*this, std::move(func));
            }

            auto until_eq(T value) {
                const auto func = [value](T elem){ return elem == value; };
                return until_enumerator<T, decltype(func)>(*this, std::move(func));
            }

            auto where_neq(T value) {
                const auto func = [value](T elem){ return elem != value; };
                return where_enumerator<T, decltype(func)>(*this, std::move(func));
            }

            std::vector<T> to_vector() {
                std::vector<T> result;
                while (*this) {
                    result.push_back(**this);
                    ++(*this);
                }
                return result;
            }

            template <typename Iter>
            void copy_to(Iter it) {
                while (static_cast<bool>(*this)) {
                    (*it)++ = **this;
                    ++(*this);
                }
            }
        };

        template <typename T, typename Iter>
        class range_enumerator : public enumerator<T> {
        public:
            range_enumerator(Iter begin, Iter end) : begin_(begin), end_(end) {}
            
            enumerator<T> &operator++() override {
                ++begin_;
                return *this;
            
            }
            operator bool() override {
                return begin_ != end_;
            }

            const T &operator*() override {
                return *begin_;
            }
        private:
            Iter begin_, end_;
        };

        template<typename T>
        class take_enumerator : public enumerator<T> {
        public:
            take_enumerator(enumerator<T>& parent, int amount) : parent_(parent), amount_(amount) {}
            
            take_enumerator& operator++() override {
                ++parent_;
                --amount_;
                return *this;
            }

            operator bool() override {
                return (amount_ > 0 && static_cast<bool>(parent_));
            }

            const T& operator*() override {
                return *parent_;
            }
        private:
            enumerator<T>& parent_;
            int amount_;
        };

        template<typename T>
        class drop_enumerator : public enumerator<T> {
        public:
            drop_enumerator(enumerator<T>& parent, int amount) : parent_(parent) {
                for (int i = 0; (i < amount) && static_cast<bool>(parent_); ++i) {
                    ++parent_;
                }
            }

            drop_enumerator& operator++() override {
                ++parent_;
                return *this;
            }

            operator bool() override {
                return static_cast<bool>(parent_);
            }

            const T& operator*() override {
                return *parent_;
            }
        private:
            enumerator<T>& parent_;
        };

        template <typename T, typename U, typename F>
        class select_enumerator : public enumerator<T> {
        public:
            select_enumerator(enumerator<U>& parent, F func) : parent_(parent), func_(std::move(func)), flag_(true) {}

            select_enumerator& operator++() override {
                ++parent_;
                flag_ = true;
                return *this;
            }

            operator bool() override {
                return static_cast<bool>(parent_);
            }

            const T& operator*() override {
                if (flag_) {
                    value_ = func_(*parent_);
                    flag_ = false;
                }
                return value_;
            }
        private:
            enumerator<U>& parent_;
            T value_;
            F func_;
            bool flag_;
        };

        template <typename T, typename F>
        class until_enumerator : public enumerator<T> {
        public:
            until_enumerator(enumerator<T> &parent, F func) : parent_(parent), func_(std::move(func)), flag_of_flag_(false) {}

            until_enumerator& operator++() override {
                ++parent_;
                flag_of_flag_ = false;
                return *this;
            }

            operator bool() override {
                if (!static_cast<bool>(parent_)) {
                    return false;
                }
                if (!flag_of_flag_) {
                    flag_ = !func_(*parent_);
                    flag_of_flag_ = true;
                }
                return flag_;
            }

            const T& operator*() override {
                return *parent_;
            }
        private:
            enumerator<T>& parent_;
            F func_;
            bool flag_;
            bool flag_of_flag_;
        };
        
        template<typename T, typename F>
        class where_enumerator : public enumerator<T> {
        public:
            where_enumerator(enumerator<T>& parent, F func) : parent_(parent), func_(std::move(func)) {
                while (static_cast<bool>(parent_) && !func_(*parent_)) {
                    ++parent_;
                }
            }
            where_enumerator& operator++() override {
                do {
                    ++parent_;
                }
                while (static_cast<bool>(parent_) && !func_(*parent_));
                return *this;
            }

            operator bool() override {
                return static_cast<bool>(parent_);
            }

            const T& operator*() override {
                return *parent_;
            }
        private:
            enumerator<T>& parent_;
            F func_;
        };

    }

    template <typename Iter>
    auto from(Iter begin, Iter end) {
        return impl::range_enumerator<typename std::iterator_traits<Iter>::value_type, Iter> (begin, end);
    }
}
