#pragma once

#include "up_pack.hpp"

namespace up_impl_ptr
{

    template <typename Ptr, void Delete(Ptr)>
    class deleter final
    {
    public: // --- operations ---
        void operator()(Ptr ptr) const
        {
            Delete(ptr);
        }
    };

    template <typename Impl, void Delete(Impl*)>
    using impl_ptr = std::unique_ptr<Impl, deleter<Impl*, Delete>>;

    template <typename Type, typename... Args>
    class impl_maker final
    {
    private: // --- state ---
        up::pack<Args&&...> _args;
    public: // --- life ---
        explicit impl_maker(Args&&... args)
            : _args(std::forward<Args>(args)...)
        { }
    public: // --- operations ---
        template <typename... Types>
        operator std::unique_ptr<Types...>() &&
        {
            return std::unique_ptr<Types...>(_create(std::index_sequence_for<Args...>()));
        }
    private:
        template <std::size_t... Indexes>
        auto _create(std::index_sequence<Indexes...>)
        {
            return new Type(std::forward<Args>(up::pack_get<Indexes>(_args))...);
        }
    };

    template <typename... Args>
    class impl_maker<void, Args...> final
    {
    private: // --- state ---
        up::pack<Args&&...> _args;
    public: // --- life ---
        explicit impl_maker(Args&&... args)
            : _args(std::forward<Args>(args)...)
        { }
    public: // --- operations ---
        template <typename Type, typename... Types>
        operator std::unique_ptr<Type, Types...>() &&
        {
            return std::unique_ptr<Type, Types...>(
                _create<Type>(std::index_sequence_for<Args...>()));
        }
    private:
        template <typename Type, std::size_t... Indexes>
        auto _create(std::index_sequence<Indexes...>)
        {
            return new Type(std::forward<Args>(up::pack_get<Indexes>(_args))...);
        }
    };

    template <typename Type = void, typename... Args>
    auto impl_make(Args&&... args) -> impl_maker<Type, Args...>
    {
        return impl_maker<Type, Args...>(std::forward<Args>(args)...);
    }

}

namespace up
{

    using up_impl_ptr::impl_ptr;
    using up_impl_ptr::impl_make;

}