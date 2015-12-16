#pragma once

#include "up_string_view.hpp"
#include "up_swap.hpp"
#include "up_variadic.hpp"

namespace up_string
{

    template <typename Result>
    auto overflow_sum_partial(Result result) -> Result
    {
        return result;
    }

    template <typename Result, typename Head, typename... Tail>
    auto overflow_sum_partial(Result result, Head head, Tail... tail) -> Result
    {
        if (__builtin_add_overflow(result, head, &result)) {
            throw std::length_error("integer overflow"); // XXX
        } else {
            return overflow_sum_partial(result, tail...);
        }
    }

    // XXX: move to separate header
    template <typename Head, typename... Tail>
    auto overflow_sum(Head head, Tail... tail)
        -> typename std::enable_if<up::variadic_all(std::is_same<Head, Tail>()...), Head>::type
    {
        return overflow_sum_partial(head, tail...);
    }


    class tags final
    {
    public: // --- scope ---
        class fill final { };
        class capacity final { };
    };


    template <typename Traits>
    class types
    {
    public: // --- scope ---
        using traits_type = Traits;
        using value_type = typename traits_type::char_type;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using const_reference = const value_type&;
        using reference = value_type&;
        using const_pointer = const value_type*;
        using pointer = value_type*;
        using const_iterator = const_pointer;
        using iterator = pointer;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using string_view = up::basic_string_view<value_type, traits_type>;
    protected:
        using const_coord_type = std::pair<const_pointer, size_type>;
        using coord_type = std::pair<pointer, size_type>;
    private:
        static_assert(std::is_same<size_type, typename string_view::size_type>(), "type mismatch");
        static_assert(std::is_same<difference_type, typename string_view::difference_type>(), "type mismatch");
        static_assert(std::is_same<const_reference, typename string_view::const_reference>(), "type mismatch");
        static_assert(std::is_same<const_pointer, typename string_view::const_pointer>(), "type mismatch");
    };


    template <typename Iterator>
    constexpr bool is_forward_iterator()
    {
        return std::is_convertible<typename std::iterator_traits<Iterator>::iterator_category, std::forward_iterator_tag>();
    }

    template <typename Traits, typename Iterator, bool IsForward>
    class iterator_fill_base;

    template <typename Traits, typename Iterator>
    class iterator_fill_base<Traits, Iterator, false>
    {
    private: // --- scope ---
        using value_type = typename types<Traits>::value_type;
        using size_type = typename types<Traits>::size_type;
    public: // --- life ---
        explicit iterator_fill_base(Iterator first, Iterator last); // XXX:IMPL
    public: // --- operations ---
        auto size() const -> size_type; // XXX:IMPL
        void apply(value_type* s) const; // XXX:IMPL
    };

    template <typename Traits, typename Iterator>
    class iterator_fill_base<Traits, Iterator, true>
    {
    private: // --- scope ---
        using value_type = typename types<Traits>::value_type;
        using size_type = typename types<Traits>::size_type;
    private: // --- state ---
        Iterator _first;
        Iterator _last;
        size_type _size;
    public: // --- life ---
        explicit iterator_fill_base(Iterator first, Iterator last)
            : _first(first), _last(last), _size(std::distance(first, last))
        { }
    public: // --- operations ---
        auto size() const -> size_type
        {
            return _size;
        }
        void apply(value_type* s) const
        {
            for (Iterator i = _first; i != _last; ++i, ++s) {
                Traits::assign(*s, *i);
            }
        }
    };


    template <typename Core, bool Mutable>
    class basic_string final : private Core
    {
    private: // --- scope ---
        using self = basic_string;

    public:
        using traits_type = typename Core::traits_type;
        using value_type = typename types<traits_type>::value_type;
        using size_type = typename types<traits_type>::size_type;
        using difference_type = typename types<traits_type>::difference_type;
        using const_reference = typename types<traits_type>::const_reference;
        using reference = typename types<traits_type>::reference;
        using const_pointer = typename types<traits_type>::const_pointer;
        using pointer = typename types<traits_type>::pointer;
        using const_iterator = typename types<traits_type>::const_iterator;
        using iterator = typename types<traits_type>::iterator;
        using const_reverse_iterator = typename types<traits_type>::const_reverse_iterator;
        using reverse_iterator = typename types<traits_type>::reverse_iterator;

        using string_view = typename types<traits_type>::string_view;

        static const constexpr size_type npos = string_view::npos;

    private:
        using const_coord_type = typename types<traits_type>::const_coord_type;
        using coord_type = typename types<traits_type>::coord_type;

        class copy_fill final
        {
        private: // --- state ---
            const value_type* _data;
            size_type _size;
        public: // --- life ---
            explicit copy_fill(const value_type* data, size_type size)
                : _data(data), _size(size)
            { }
        public: // --- operations ---
            auto size() const -> size_type
            {
                return _size;
            }
            void apply(value_type* s) const
            {
                traits_type::copy(s, _data, _size);
            }
        };

        class assign_fill final
        {
        private: // --- state ---
            size_type _size;
            value_type _value;
        public: // --- life ---
            explicit assign_fill(size_type size, value_type value)
                : _size(size), _value(value)
            { }
        public: // --- operations ---
            auto size() const -> size_type
            {
                return _size;
            }
            void apply(value_type* s) const
            {
                traits_type::assign(s, _size, _value);
            }
        };

        class value_fill final
        {
        private: // --- state ---
            value_type _value;
        public: // --- life ---
            explicit value_fill(value_type value)
                : _value(value)
            { }
        public: // --- operations ---
            auto size() const -> size_type
            {
                return 1;
            }
            void apply(value_type* s) const
            {
                traits_type::assign(*s, _value);
            }
        };

        template <typename Iterator>
        class iterator_fill final : public iterator_fill_base<traits_type, Iterator, is_forward_iterator<Iterator>()>
        {
        private: // --- scope ---
            using base = iterator_fill_base<traits_type, Iterator, is_forward_iterator<Iterator>()>;
        public: // --- life ---
            explicit iterator_fill(Iterator first, Iterator last)
                : base(first, last)
            { }
        };

        static auto _make_fill(const up::string_view& s) -> copy_fill
        {
            return copy_fill(s.data(), s.size());
        }

        static void _apply_fills(value_type* ptr __attribute__((unused))) { }
        template <typename Head, typename... Tail>
        static void _apply_fills(value_type* ptr, Head&& head, Tail&&... tail)
        {
            size_type size = head.size();
            std::forward<Head>(head).apply(ptr);
            _apply_fills(ptr + size, std::forward<Tail>(tail)...);
        }

        static void _range_check(bool condition, const char* message)
        {
            if (!condition) {
                throw std::out_of_range(message);
            }
        }
        static auto _const_iterator(const_pointer ptr, size_type pos) -> const_iterator
        {
            return const_iterator(ptr + pos);
        }
        static auto _iterator(pointer ptr, size_type pos) -> iterator
        {
            return iterator(ptr + pos);
        }

    public:
        template <typename... Args>
        static auto concat(Args&&... args) -> self
        {
            return self(tags::fill(), {}, {}, _make_fill(std::forward<Args>(args))...);
        }

    public: // --- life ---
        basic_string() noexcept = default;
        basic_string(const self& rhs) = default;
        basic_string(self&& rhs) noexcept = default;
        ~basic_string() noexcept = default;

        basic_string(const string_view& s)
            : basic_string(s.data(), s.size())
        { }
        basic_string(const string_view& s, size_type pos, size_type n = npos)
            : basic_string(s.substr(pos, n))
        { }
        basic_string(const value_type* s)
            : basic_string(s, traits_type::length(s))
        { }
        basic_string(const value_type* s, size_type n)
            : basic_string(tags::fill(), {}, {}, copy_fill(s, n))
        { }
        basic_string(size_type n, value_type c)
            : basic_string(tags::fill(), {}, {}, assign_fill(n, c))
        { }
        template <typename InputIterator>
        basic_string(InputIterator first, InputIterator last)
            : basic_string(tags::fill(), {}, {}, iterator_fill<InputIterator>(first, last))
        { }
        basic_string(std::initializer_list<value_type> chars)
            : basic_string(chars.begin(), chars.end())
        { }
    private:
        template <typename... Fills>
        explicit basic_string(tags::fill, size_type baseline, size_type headroom, Fills&&... fills)
            : basic_string(tags::capacity(), baseline, headroom, overflow_sum(headroom, fills.size()...))
        {
            _apply_fills(_coord().first, std::forward<Fills>(fills)...);
        }
        explicit basic_string(tags::capacity, size_type baseline, size_type headroom, size_type request)
            : Core(tags::capacity(), std::max(request, baseline + baseline / 2 + 32), request - headroom)
        { }

    public: // --- operations ---
        auto operator=(const self& rhs) & -> self& = default;
        auto operator=(self&& rhs) & noexcept -> self& = default;

        // MUTATION
        auto operator=(const string_view& s) -> self&
        {
            return assign(s);
        }
        // MUTATION
        auto operator=(const value_type* s) -> self&
        {
            return assign(s);
        }
        // MUTATION
        auto operator=(value_type c) -> self&
        {
            return assign(size_type(1), c);
        }
        // MUTATION
        auto operator=(std::initializer_list<value_type> chars) -> self&
        {
            return assign(chars.begin(), chars.end());
        }

        auto begin() const noexcept -> const_iterator
        {
            return cbegin();
        }
        // MUTATION
        auto begin() noexcept -> iterator
        {
            auto coord = _coord();
            return _iterator(coord.first, {});
        }
        auto end() const noexcept -> const_iterator
        {
            return cend();
        }
        // MUTATION
        auto end() noexcept -> iterator
        {
            auto coord = _coord();
            return _iterator(coord.first, coord.second);
        }

        auto rbegin() const noexcept -> const_reverse_iterator
        {
            return crbegin();
        }
        // MUTATION
        auto rbegin() noexcept -> reverse_iterator
        {
            return reverse_iterator(end());
        }
        auto rend() const noexcept -> const_reverse_iterator
        {
            return crend();
        }
        // MUTATION
        auto rend() noexcept -> reverse_iterator
        {
            return reverse_iterator(begin());
        }

        auto cbegin() const noexcept -> const_iterator
        {
            auto coord = _const_coord();
            return _const_iterator(coord.first, {});
        }
        auto cend() const noexcept -> const_iterator
        {
            auto coord = _const_coord();
            return _const_iterator(coord.first, coord.second);
        }
        auto crbegin() const noexcept -> const_reverse_iterator
        {
            return const_reverse_iterator(cend());
        }
        auto crend() const noexcept -> const_reverse_iterator
        {
            return const_reverse_iterator(cbegin());
        }

        auto size() const noexcept -> size_type
        {
            return _size();
        }
        auto length() const noexcept -> size_type
        {
            return _size();
        }
        auto max_size() const noexcept -> size_type
        {
            return Core::max_size();
        }
        // MUTATION
        void resize(size_type n, value_type c)
        {
            auto size = _size();
            if (n > size) {
                append(n - size, c);
            } else {
                _trim_size(n);
            }
        }
        // MUTATION
        void resize(size_type n)
        {
            resize(n, {});
        }
        auto capacity() const noexcept -> size_type
        {
            return _capacity();
        }
        // MUTATION
        void reserve(size_type request = {})
        {
            auto capacity = _capacity();
            if (request > capacity) {
                auto coord = _coord();
                self(tags::fill(), {}, request - coord.second,
                    copy_fill(coord.first, coord.second)).swap(*this);
            } else if (capacity - request < sizeof(self)) {
                // nothing
            } else if (capacity - _size() < sizeof(self)) {
                // nothing
            } else {
                auto coord = _coord();
                self(tags::fill(), {}, request - coord.second,
                    copy_fill(coord.first, coord.second)).swap(*this);
            }
        }
        // MUTATION
        void shrink_to_fit()
        {
            if (_capacity() - _size() < sizeof(self)) {
                // nothing
            } else {
                self(*this).swap(*this);
            }
        }
        // MUTATION
        void clear() noexcept
        {
            _trim_size({});
        }
        bool empty() const noexcept
        {
            return !_size();
        }

        auto operator[](size_type pos) const -> const_reference
        {
            auto coord = _const_coord();
            return coord.first[pos];
        }
        // MUTATION
        auto operator[](size_type pos) -> reference
        {
            auto coord = _coord();
            return coord.first[pos];
        }
        auto at(size_type n) const -> const_reference
        {
            auto coord = _const_coord();
            _range_check(n < coord.second, "up::basic_string::at: position out of bounds");
            return coord.first[n];
        }
        // MUTATION
        auto at(size_type n) -> reference
        {
            auto coord = _coord();
            _range_check(n < coord.second, "up::basic_string::at: position out of bounds");
            return coord.first[n];
        }

        auto front() const -> const_reference
        {
            auto coord = _const_coord();
            return coord.first[0];
        }
        // MUTATION
        auto front() -> reference
        {
            auto coord = _coord();
            return coord.first[0];
        }
        auto back() const -> const_reference
        {
            auto coord = _const_coord();
            return coord.first[coord.second - size_type(1)];
        }
        // MUTATION
        auto back() -> reference
        {
            auto coord = _coord();
            return coord.first[coord.second - size_type(1)];
        }

        // MUTATION
        auto operator+=(const string_view& s) -> self&
        {
            return append(s);
        }
        // MUTATION
        auto operator+=(const value_type* s) -> self&
        {
            return append(s);
        }
        // MUTATION
        auto operator+=(value_type c) -> self&
        {
            return append(size_type(1), c);
        }
        // MUTATION
        auto operator+=(std::initializer_list<value_type> chars) -> self&
        {
            return append(chars);
        }

        // MUTATION
        auto append(const string_view& s) -> self&
        {
            return append(s.data(), s.size());
        }
        // MUTATION
        auto append(const string_view& s, size_type pos, size_type n) -> self&
        {
            return append(s.substr(pos, n));
        }
        // MUTATION
        auto append(const value_type* s, size_type n) -> self&
        {
            return _append_fill(copy_fill(s, n));
        }
        // MUTATION
        auto append(const value_type* s) -> self&
        {
            return append(s, traits_type::length(s));
        }
        // MUTATION
        auto append(size_type n, value_type c) -> self&
        {
            return _append_fill(assign_fill(n, c));
        }
        // MUTATION
        template <typename InputIterator>
        auto append(InputIterator first, InputIterator last) -> self&
        {
            return _append_fill(iterator_fill<InputIterator>(first, last));
        }
        // MUTATION
        auto append(std::initializer_list<value_type> chars) -> self&
        {
            return append(chars.begin(), chars.end());
        }

        // MUTATION
        void push_back(value_type c)
        {
            _append_fill(value_fill(c));
        }

        auto assign(const self& rhs) -> self&
        {
            return operator=(rhs);
        }
        auto assign(self&& rhs) noexcept -> self&
        {
            return operator=(std::forward<self>(rhs));
        }

        // MUTATION
        auto assign(const string_view& s) -> self&
        {
            return assign(s.data(), s.size());
        }
        // MUTATION
        auto assign(const string_view& s, size_type pos, size_type n) -> self&
        {
            return assign(s.substr(pos, n));
        }
        // MUTATION
        auto assign(const value_type* s, size_type n) -> self&
        {
            return _assign_fill(copy_fill(s, n));
        }
        // MUTATION
        auto assign(const value_type* s) -> self&
        {
            return assign(s, traits_type::length(s));
        }
        // MUTATION
        auto assign(size_type n, value_type c) -> self&
        {
            return _assign_fill(assign_fill(n, c));
        }
        // MUTATION
        template <typename InputIterator>
        auto assign(InputIterator first, InputIterator last) -> self&
        {
            return _assign_fill(iterator_fill<InputIterator>(first, last));
        }
        // MUTATION
        auto assign(std::initializer_list<value_type> chars) -> self&
        {
            return assign(chars.begin(), chars.end());
        }

        // MUTATION
        auto insert(size_type pos1, const string_view& s) -> self&
        {
            return insert(pos1, s.data(), s.size());
        }
        // MUTATION
        auto insert(size_type pos1, const string_view& s, size_type pos2, size_type n) -> self&
        {
            return insert(pos1, s.substr(pos2, n));
        }
        // MUTATION
        auto insert(size_type pos, const value_type* s, size_type n) -> self&
        {
            return _insert_fill(pos, copy_fill(s, n));
        }
        // MUTATION
        auto insert(size_type pos, const value_type* s) -> self&
        {
            return insert(pos, s, traits_type::length(s));
        }
        // MUTATION
        auto insert(size_type pos, size_type n, value_type c) -> self&
        {
            return _insert_fill(pos, assign_fill(n, c));
        }
        // MUTATION
        auto insert(const_iterator p, value_type c) -> iterator
        {
            return _insert_fill(p, value_fill(c));
        }
        // MUTATION
        auto insert(const_iterator p, size_type n, value_type c) -> iterator
        {
            return _insert_fill(p, assign_fill(n, c));
        }
        // MUTATION
        template <typename InputIterator>
        auto insert(const_iterator p, InputIterator first, InputIterator last) -> iterator
        {
            return _insert_fill(p, iterator_fill<InputIterator>(first, last));
        }
        // MUTATION
        auto insert(const_iterator p, std::initializer_list<value_type> chars) -> iterator
        {
            return insert(p, chars.begin(), chars.end());
        }

        // MUTATION
        auto erase(size_type pos = {}, size_type n = npos) -> self&
        {
            auto size = _size();
            _range_check(pos <= size, "up::basic_string::erase: position out of bounds");
            return _erase(pos, std::min(n, size - pos));
        }
        // MUTATION
        auto erase(const_iterator position) -> iterator
        {
            return erase(position, std::next(position));
        }
        // MUTATION
        auto erase(const_iterator first, const_iterator last) -> iterator
        {
            auto coord = _coord();
            size_type pos = std::distance(_const_iterator(coord.first, {}), first);
            size_type n = std::distance(first, last);
            _erase(pos, n);
            return _iterator(coord.first, pos);
        }

        // MUTATION
        void pop_back()
        {
            _trim_size(_size() - size_type(1));
        }

        // MUTATION
        auto replace(size_type pos1, size_type n1, const string_view& s) -> self&
        {
            return replace(pos1, n1, s.data(), s.size());
        }
        // MUTATION
        auto replace(size_type pos1, size_type n1, const string_view& s, size_type pos2, size_type n2) -> self&
        {
            return replace(pos1, n1, s.substr(pos2, n2));
        }
        // MUTATION
        auto replace(size_type pos, size_type n1, const value_type* s, size_type n2) -> self&
        {
            return _replace_fill(pos, n1, copy_fill(s, n2));
        }
        // MUTATION
        auto replace(size_type pos, size_type n1, const value_type* s) -> self&
        {
            return replace(pos, n1, s, traits_type::length(s));
        }
        // MUTATION
        auto replace(size_type pos, size_type n1, size_type n2, value_type c) -> self&
        {
            return _replace_fill(pos, n1, assign_fill(n2, c));
        }
        // MUTATION
        auto replace(const_iterator p, const_iterator q, const string_view& s) -> self&
        {
            return replace(p, q, s.data(), s.size());
        }
        // MUTATION
        auto replace(const_iterator p, const_iterator q, const value_type* s, size_type n) -> self&
        {
            return _replace_fill(p, q, copy_fill(s, n));
        }
        // MUTATION
        auto replace(const_iterator p, const_iterator q, const value_type* s) -> self&
        {
            return replace(p, q, s, traits_type::length(s));
        }
        // MUTATION
        auto replace(const_iterator p, const_iterator q, size_type n, value_type c) -> self&
        {
            return _replace_fill(p, q, assign_fill(n, c));
        }
        // MUTATION
        template <typename InputIterator>
        auto replace(const_iterator p, const_iterator q, InputIterator first, InputIterator last) -> self&
        {
            return _replace_fill(p, q, iterator_fill<InputIterator>(first, last));
        }
        // MUTATION
        auto replace(const_iterator p, const_iterator q, std::initializer_list<value_type> chars) -> self&
        {
            return replace(p, q, chars.begin(), chars.end());
        }

        auto copy(value_type* s, size_type n, size_type pos = {}) const -> size_type
        {
            return operator string_view().copy(s, n, pos);
        }
        void swap(self& rhs) noexcept
        {
            Core::swap(rhs);
        }
        friend void swap(self& lhs, self& rhs) noexcept
        {
            lhs.swap(rhs);
        }

        auto data() const noexcept -> const value_type*
        {
            return _const_coord().first;
        }
        auto c_str() const noexcept -> const value_type* = delete; // intentionally not supported

        auto find(const string_view& s, size_type pos = {}) const noexcept -> size_type
        {
            return operator string_view().find(s, pos);
        }
        auto find(const value_type* s, size_type pos, size_type n) const noexcept -> size_type
        {
            return operator string_view().find(s, pos, n);
        }
        auto find(const value_type* s, size_type pos = {}) const noexcept -> size_type
        {
            return operator string_view().find(s, pos);
        }
        auto find(value_type c, size_type pos = {}) const noexcept -> size_type
        {
            return operator string_view().find(c, pos);
        }

        auto rfind(const string_view& s, size_type pos = npos) const noexcept -> size_type
        {
            return operator string_view().rfind(s, pos);
        }
        auto rfind(const value_type* s, size_type pos, size_type n) const noexcept -> size_type
        {
            return operator string_view().rfind(s, pos, n);
        }
        auto rfind(const value_type* s, size_type pos = npos) const noexcept -> size_type
        {
            return operator string_view().rfind(s, pos);
        }
        auto rfind(value_type c, size_type pos = npos) const noexcept -> size_type
        {
            return operator string_view().rfind(c, pos);
        }

        auto find_first_of(const string_view& s, size_type pos = {}) const noexcept -> size_type
        {
            return operator string_view().find_first_of(s, pos);
        }
        auto find_first_of(const value_type* s, size_type pos, size_type n) const noexcept -> size_type
        {
            return operator string_view().find_first_of(s, pos, n);
        }
        auto find_first_of(const value_type* s, size_type pos = {}) const noexcept -> size_type
        {
            return operator string_view().find_first_of(s, pos);
        }
        auto find_first_of(value_type c, size_type pos = {}) const noexcept -> size_type
        {
            return operator string_view().find_first_of(c, pos);
        }

        auto find_last_of(const string_view& s, size_type pos = npos) const noexcept -> size_type
        {
            return operator string_view().find_last_of(s, pos);
        }
        auto find_last_of(const value_type* s, size_type pos, size_type n) const noexcept -> size_type
        {
            return operator string_view().find_last_of(s, pos, n);
        }
        auto find_last_of(const value_type* s, size_type pos = npos) const noexcept -> size_type
        {
            return operator string_view().find_last_of(s, pos);
        }
        auto find_last_of(value_type c, size_type pos = npos) const noexcept -> size_type
        {
            return operator string_view().find_last_of(c, pos);
        }

        auto find_first_not_of(const string_view& s, size_type pos = {}) const noexcept -> size_type
        {
            return operator string_view().find_first_not_of(s, pos);
        }
        auto find_first_not_of(const value_type* s, size_type pos, size_type n) const noexcept -> size_type
        {
            return operator string_view().find_first_not_of(s, pos, n);
        }
        auto find_first_not_of(const value_type* s, size_type pos = {}) const noexcept -> size_type
        {
            return operator string_view().find_first_not_of(s, pos);
        }
        auto find_first_not_of(value_type c, size_type pos = {}) const noexcept -> size_type
        {
            return operator string_view().find_first_not_of(c, pos);
        }

        auto find_last_not_of(const string_view& s, size_type pos = npos) const noexcept -> size_type
        {
            return operator string_view().find_last_not_of(s, pos);
        }
        auto find_last_not_of(const value_type* s, size_type pos, size_type n) const noexcept -> size_type
        {
            return operator string_view().find_last_not_of(s, pos, n);
        }
        auto find_last_not_of(const value_type* s, size_type pos = npos) const noexcept -> size_type
        {
            return operator string_view().find_last_not_of(s, pos);
        }
        auto find_last_not_of(value_type c, size_type pos = npos) const noexcept -> size_type
        {
            return operator string_view().find_last_not_of(c, pos);
        }

        auto substr(size_type pos = {}, size_type n = npos) const -> self
        {
            auto coord = _const_coord();
            _range_check(pos <= coord.second, "up::basic_string::subtr: position out of bounds");
            return self(tags::fill(), {}, {},
                copy_fill(coord.first + pos, std::min(n, coord.second - pos)));
        }

        auto compare(const string_view& s) const noexcept -> int
        {
            return operator string_view().compare(s);
        }
        auto compare(size_type pos1, size_type n1, const string_view& s) const -> int
        {
            return operator string_view().compare(pos1, n1, s);
        }
        auto compare(size_type pos1, size_type n1, const string_view& s, size_type pos2, size_type n2) const -> int
        {
            return operator string_view().compare(pos1, n1, s, pos2, n2);
        }
        auto compare(const value_type* s) const noexcept -> int
        {
            return operator string_view().compare(s);
        }
        auto compare(size_type pos1, size_type n1, const value_type* s) const -> int
        {
            return operator string_view().compare(pos1, n1, s);
        }
        auto compare(size_type pos1, size_type n1, const value_type* s, size_type n2) const -> int
        {
            return operator string_view().compare(pos1, n1, s, n2);
        }

        operator string_view() const noexcept
        {
            auto coord = _const_coord();
            return {coord.first, coord.second};
        }
        // XXX: temporary workaround for up::to_string
        auto to_string() const -> std::string
        {
            auto coord = _const_coord();
            return {coord.first, coord.second};
        }
        // XXX: temporary workaround for up::out
        void out(std::ostream& os) const
        {
            auto coord = _const_coord();
            os.write(coord.first, coord.second);
        }

    private:
        auto _const_coord() const noexcept -> const_coord_type
        {
            return Core::const_coord();
        }
        auto _coord() noexcept -> coord_type
        {
            return Core::coord();
        }
        auto _size() const noexcept -> size_type
        {
            return Core::size();
        }
        auto _capacity() const -> size_type
        {
            return Core::capacity();
        }
        bool _increase_size(size_type n)
        {
            size_type size = overflow_sum(_size(), n);
            if (size <= _capacity()) {
                Core::set_size(size);
                return true;
            } else {
                return false;
            }
        }
        void _trim_size(size_type n)
        {
            Core::set_size(n);
        }
        template <typename Fill>
        auto _append_fill(Fill&& fill) -> self&
        {
            auto coord = _coord();
            size_type n = fill.size();
            if (_increase_size(n)) {
                fill.apply(coord.first + coord.second);
            } else {
                self(tags::fill(), _capacity(), {},
                    copy_fill(coord.first, coord.second),
                    std::forward<Fill>(fill)).swap(*this);
            }
            return *this;
        }
        template <typename Fill>
        auto _assign_fill(Fill&& fill) -> self&
        {
            auto coord = _coord();
            size_type n = fill.size();
            if (n <= coord.second) {
                fill.apply(coord.first);
                _trim_size(n);
            } else if (_increase_size(n - coord.second)) {
                fill.apply(coord.first);
            } else {
                self(tags::fill(), {}, {}, std::forward<Fill>(fill)).swap(*this);
            }
            return *this;
        }
        template <typename Fill>
        auto _insert_fill(size_type pos, Fill&& fill) -> self&
        {
            auto coord = _coord();
            _range_check(pos <= coord.second, "up::basic_string::insert: position out of bounds");
            size_type n = fill.size();
            if (_increase_size(n)) {
                traits_type::move(coord.first + pos + n, coord.first + pos, coord.second - pos);
                fill.apply(coord.first + pos);
            } else {
                self(tags::fill(), _capacity(), {},
                    copy_fill(coord.first, pos),
                    std::forward<Fill>(fill),
                    copy_fill(coord.first + pos, coord.second - pos)).swap(*this);
            }
            return *this;
        }
        template <typename Fill>
        auto _insert_fill(const_iterator p, Fill&& fill) -> iterator
        {
            size_type pos = std::distance(_const_iterator(_coord().first, {}), p);
            _insert_fill(pos, std::forward<Fill>(fill));
            return _iterator(_coord().first, pos);
        }
        template <typename Fill>
        auto _replace_fill(size_type pos, size_type n, Fill&& fill) -> self&
        {
            auto coord = _coord();
            size_type k = fill.size();
            if (k <= n) {
                traits_type::move(coord.first + pos + k, coord.first + pos + n, coord.second - pos - n);
                fill.apply(coord.first + pos);
                _trim_size(coord.second + (n - k));
            } else if (_increase_size(k - n)) {
                traits_type::move(coord.first + pos + k, coord.first + pos + n, coord.second - pos - n);
                fill.apply(coord.first + pos);
            } else {
                self(tags::fill(), _capacity(), {},
                    copy_fill(coord.first, pos),
                    std::forward<Fill>(fill),
                    copy_fill(coord.first + pos + n, coord.second - pos - n)).swap(*this);
            }
            return *this;
        }
        template <typename Fill>
        auto _replace_fill(const_iterator p, const_iterator q, Fill&& fill) -> self&
        {
            size_type pos = std::distance(_const_iterator(_coord().first, {}), p);
            size_type n = std::distance(p, q);
            return _replace_fill(pos, n, std::forward<Fill>(fill));
        }
        auto _erase(size_type pos, size_type n) -> self&
        {
            if (n) {
                auto coord = _coord();
                traits_type::move(coord.first + pos, coord.first + pos + n, coord.second - pos - n);
                _trim_size(coord.second - n);
            } // else: nothing
            return *this;
        }
    };


    template <typename Core, bool Mutable>
    auto operator+(
        const basic_string<Core, Mutable>& lhs,
        const basic_string<Core, Mutable>& rhs)
        -> basic_string<Core, Mutable>
    {
        return basic_string<Core, Mutable>::concat(lhs, rhs);
    }

    template <typename Core, bool Mutable>
    auto operator+(
        const basic_string<Core, Mutable>& lhs,
        const typename basic_string<Core, Mutable>::string_view& rhs)
        -> basic_string<Core, Mutable>
    {
        return basic_string<Core, Mutable>::concat(lhs, rhs);
    }

    template <typename Core, bool Mutable>
    auto operator+(
        const basic_string<Core, Mutable>& lhs,
        const typename basic_string<Core, Mutable>::value_type* rhs)
        -> basic_string<Core, Mutable>
    {
        return basic_string<Core, Mutable>::concat(lhs, rhs);
    }

    template <typename Core, bool Mutable>
    auto operator+(
        const basic_string<Core, Mutable>& lhs,
        typename basic_string<Core, Mutable>::value_type rhs)
        -> basic_string<Core, Mutable>
    {
        return basic_string<Core, Mutable>::concat(lhs, rhs);
    }

    template <typename Core, bool Mutable>
    auto operator+(
        const typename basic_string<Core, Mutable>::string_view& lhs,
        const basic_string<Core, Mutable>& rhs)
        -> basic_string<Core, Mutable>
    {
        return basic_string<Core, Mutable>::concat(lhs, rhs);
    }

    template <typename Core, bool Mutable>
    auto operator+(
        const typename basic_string<Core, Mutable>::value_type* lhs,
        const basic_string<Core, Mutable>& rhs)
        -> basic_string<Core, Mutable>
    {
        return basic_string<Core, Mutable>::concat(lhs, rhs);
    }

    template <typename Core, bool Mutable>
    auto operator+(
        typename basic_string<Core, Mutable>::value_type lhs,
        const basic_string<Core, Mutable>& rhs)
        -> basic_string<Core, Mutable>
    {
        return basic_string<Core, Mutable>::concat(lhs, rhs);
    }


    template <typename Core>
    auto operator+(
        basic_string<Core, true>&& lhs,
        const basic_string<Core, true>& rhs)
        -> basic_string<Core, true>
    {
        return std::move(lhs.append(rhs));
    }

    template <typename Core>
    auto operator+(
        const basic_string<Core, true>& lhs,
        basic_string<Core, true>&& rhs)
        -> basic_string<Core, true>
    {
        return std::move(rhs.insert(0, lhs));
    }

    template <typename Core>
    auto operator+(
        basic_string<Core, true>&& lhs,
        basic_string<Core, true>&& rhs)
        -> basic_string<Core, true>
    {
        return std::move(lhs.append(rhs));
    }

    template <typename Core>
    auto operator+(
        basic_string<Core, true>&& lhs,
        const typename basic_string<Core, true>::string_view& rhs)
        -> basic_string<Core, true>
    {
        return std::move(lhs.append(rhs));
    }

    template <typename Core>
    auto operator+(
        basic_string<Core, true>&& lhs,
        const typename basic_string<Core, true>::value_type* rhs)
        -> basic_string<Core, true>
    {
        return std::move(lhs.append(rhs));
    }

    template <typename Core>
    auto operator+(
        basic_string<Core, true>&& lhs,
        typename basic_string<Core, true>::value_type rhs)
        -> basic_string<Core, true>
    {
        return std::move(lhs.append(&rhs, 1));
    }

    template <typename Core>
    auto operator+(
        const typename basic_string<Core, true>::string_view& lhs,
        basic_string<Core, true>&& rhs)
        -> basic_string<Core, true>
    {
        return std::move(rhs.insert(0, lhs));
    }

    template <typename Core>
    auto operator+(
        const typename basic_string<Core, true>::value_type* lhs,
        basic_string<Core, true>&& rhs)
        -> basic_string<Core, true>
    {
        return std::move(rhs.insert(0, lhs));
    }

    template <typename Core>
    auto operator+(
        typename basic_string<Core, true>::value_type lhs,
        basic_string<Core, true>&& rhs)
        -> basic_string<Core, true>
    {
        return std::move(rhs.insert(0, &lhs, 1));
    }


    template <typename Core, bool Mutable>
    bool operator==(
        const basic_string<Core, Mutable>& lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() == rhs.operator string_view();
    }

    template <typename Core, bool Mutable>
    bool operator==(
        const basic_string<Core, Mutable>& lhs,
        const typename basic_string<Core, Mutable>::string_view& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() == rhs;
    }

    template <typename Core, bool Mutable>
    bool operator==(
        const basic_string<Core, Mutable>& lhs,
        const typename basic_string<Core, Mutable>::value_type* rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() == rhs;
    }

    template <typename Core, bool Mutable>
    bool operator==(
        const typename basic_string<Core, Mutable>::string_view& lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs == rhs.operator string_view();
    }

    template <typename Core, bool Mutable>
    bool operator==(
        const typename basic_string<Core, Mutable>::value_type* lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs == rhs.operator string_view();
    }


    template <typename Core, bool Mutable>
    bool operator!=(
        const basic_string<Core, Mutable>& lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() != rhs.operator string_view();
    }

    template <typename Core, bool Mutable>
    bool operator!=(
        const basic_string<Core, Mutable>& lhs,
        const typename basic_string<Core, Mutable>::string_view& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() != rhs;
    }

    template <typename Core, bool Mutable>
    bool operator!=(
        const basic_string<Core, Mutable>& lhs,
        const typename basic_string<Core, Mutable>::value_type* rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() != rhs;
    }

    template <typename Core, bool Mutable>
    bool operator!=(
        const typename basic_string<Core, Mutable>::string_view& lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs != rhs.operator string_view();
    }

    template <typename Core, bool Mutable>
    bool operator!=(
        const typename basic_string<Core, Mutable>::value_type* lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs != rhs.operator string_view();
    }


    template <typename Core, bool Mutable>
    bool operator<(
        const basic_string<Core, Mutable>& lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() < rhs.operator string_view();
    }

    template <typename Core, bool Mutable>
    bool operator<(
        const basic_string<Core, Mutable>& lhs,
        const typename basic_string<Core, Mutable>::string_view& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() < rhs;
    }

    template <typename Core, bool Mutable>
    bool operator<(
        const basic_string<Core, Mutable>& lhs,
        const typename basic_string<Core, Mutable>::value_type* rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() < rhs;
    }

    template <typename Core, bool Mutable>
    bool operator<(
        const typename basic_string<Core, Mutable>::string_view& lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs < rhs.operator string_view();
    }

    template <typename Core, bool Mutable>
    bool operator<(
        const typename basic_string<Core, Mutable>::value_type* lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs < rhs.operator string_view();
    }


    template <typename Core, bool Mutable>
    bool operator>(
        const basic_string<Core, Mutable>& lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() > rhs.operator string_view();
    }

    template <typename Core, bool Mutable>
    bool operator>(
        const basic_string<Core, Mutable>& lhs,
        const typename basic_string<Core, Mutable>::string_view& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() > rhs;
    }

    template <typename Core, bool Mutable>
    bool operator>(
        const basic_string<Core, Mutable>& lhs,
        const typename basic_string<Core, Mutable>::value_type* rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() > rhs;
    }

    template <typename Core, bool Mutable>
    bool operator>(
        const typename basic_string<Core, Mutable>::string_view& lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs > rhs.operator string_view();
    }

    template <typename Core, bool Mutable>
    bool operator>(
        const typename basic_string<Core, Mutable>::value_type* lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs > rhs.operator string_view();
    }


    template <typename Core, bool Mutable>
    bool operator<=(
        const basic_string<Core, Mutable>& lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() <= rhs.operator string_view();
    }

    template <typename Core, bool Mutable>
    bool operator<=(
        const basic_string<Core, Mutable>& lhs,
        const typename basic_string<Core, Mutable>::string_view& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() <= rhs;
    }

    template <typename Core, bool Mutable>
    bool operator<=(
        const basic_string<Core, Mutable>& lhs,
        const typename basic_string<Core, Mutable>::value_type* rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() <= rhs;
    }

    template <typename Core, bool Mutable>
    bool operator<=(
        const typename basic_string<Core, Mutable>::string_view& lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs <= rhs.operator string_view();
    }

    template <typename Core, bool Mutable>
    bool operator<=(
        const typename basic_string<Core, Mutable>::value_type* lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs <= rhs.operator string_view();
    }


    template <typename Core, bool Mutable>
    bool operator>=(
        const basic_string<Core, Mutable>& lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() >= rhs.operator string_view();
    }

    template <typename Core, bool Mutable>
    bool operator>=(
        const basic_string<Core, Mutable>& lhs,
        const typename basic_string<Core, Mutable>::string_view& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() >= rhs;
    }

    template <typename Core, bool Mutable>
    bool operator>=(
        const basic_string<Core, Mutable>& lhs,
        const typename basic_string<Core, Mutable>::value_type* rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs.operator string_view() >= rhs;
    }

    template <typename Core, bool Mutable>
    bool operator>=(
        const typename basic_string<Core, Mutable>::string_view& lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs >= rhs.operator string_view();
    }

    template <typename Core, bool Mutable>
    bool operator>=(
        const typename basic_string<Core, Mutable>::value_type* lhs,
        const basic_string<Core, Mutable>& rhs) noexcept
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return lhs >= rhs.operator string_view();
    }


    template <typename Core, bool Mutable>
    auto operator<<(
        std::basic_ostream<typename Core::traits_type::char_type, typename Core::traits_type>& os,
        const basic_string<Core, Mutable>& s)
        -> std::basic_ostream<typename Core::traits_type::char_type, typename Core::traits_type>&
    {
        using string_view = typename basic_string<Core, Mutable>::string_view;
        return os << s.operator string_view();
    }


    class core : protected types<std::char_traits<char>>
    {
    private: // --- scope ---
        using self = core;
    protected:
        static auto max_size() -> size_type
        {
            return std::numeric_limits<size_type>::max();
        }
    private: // --- state ---
        size_type _capacity;
        size_type _size;
        std::unique_ptr<char[]> _data;
    protected: // --- life ---
        explicit core() noexcept
            : _capacity(), _size()
        { }
        core(const self& rhs)
            : _capacity(rhs._size), _size(rhs._size), _data(std::make_unique<char[]>(rhs._size))
        {
            traits_type::copy(_data.get(), rhs._data.get(), rhs._size);
        }
        core(self&& rhs) noexcept
            : core()
        {
            swap(rhs);
        }
        ~core() noexcept = default;
        explicit core(tags::capacity, size_type capacity, size_type size)
            : _capacity(capacity), _size(size), _data(std::make_unique<char[]>(capacity))
        { }
    protected: // --- operations ---
        auto operator=(const self& rhs) & -> self&
        {
            if (this == &rhs) {
                // nothing (required for following condition)
            } else if (_capacity >= rhs._size) {
                traits_type::copy(_data.get(), rhs._data.get(), rhs._size);
                _size = rhs._size;
            } else {
                self(rhs).swap(*this);
            }
            return *this;
        }
        auto operator=(self&& rhs) & noexcept -> self&
        {
            swap(rhs);
            return *this;
        }
        void swap(self& rhs) noexcept
        {
            up::swap_noexcept(_capacity, rhs._capacity);
            up::swap_noexcept(_size, rhs._size);
            up::swap_noexcept(_data, rhs._data);
        }
        auto const_coord() const noexcept -> const_coord_type
        {
            return {_data.get(), _size};
        }
        auto coord() noexcept -> coord_type
        {
            return {_data.get(), _size};
        }
        auto capacity() const noexcept -> size_type
        {
            return _capacity;
        }
        auto size() const noexcept -> size_type
        {
            return _size;
        }
        void set_size(size_type n) noexcept
        {
            _size = n;
        }
    };

    using string = basic_string<core, true>;

    extern template class basic_string<core, true>;

}

namespace std
{

    template <typename Core, bool Mutable>
    struct hash<up_string::basic_string<Core, Mutable>> final
    {
        using argument_type = up_string::basic_string<Core, Mutable>;
        using result_type = std::size_t;
        auto operator()(const up_string::basic_string<Core, Mutable>& value) const noexcept
        {
            using string_view = typename up_string::basic_string<Core, Mutable>::string_view;
            return hash<string_view>()(value.operator string_view());
        }
    };

}

namespace up
{

    using up_string::basic_string;
    using up_string::string;

    using shared_string = up::string; // XXX
    using unique_string = up::string; // XXX

}