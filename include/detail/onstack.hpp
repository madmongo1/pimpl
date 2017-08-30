// Copyright (c) 2008-2018 Vladimir Batov.
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. See http://www.boost.org/LICENSE_1_0.txt.

#ifndef IMPL_PTR_DETAIL_ONSTACK_HPP
#define IMPL_PTR_DETAIL_ONSTACK_HPP

#include "./detail.hpp"

namespace impl_ptr_policy
{
    template<typename, typename =void> struct onstack;
}

template<typename impl_type, typename size_type>
struct impl_ptr_policy::onstack // Proof of concept
{
    template<typename T =void> struct allocator : std::allocator<T>
    {
        void deallocate(T*, size_t) {}

        template<typename Y> struct rebind { using other = allocator<Y>; };
    };
    using    this_type = onstack;
    using storage_type = boost::aligned_storage<sizeof(size_type)>;
    using  traits_type = detail::traits::copyable<impl_type, allocator<>>;
    using   traits_ptr = typename traits_type::pointer;

   ~onstack () { if (traits_) traits_->destroy(get()); }
    onstack (std::nullptr_t) {}
    onstack (this_type const& o) : traits_(o.traits_)
    {
        if (traits_)
            traits_->construct(storage_.address(), *o.get());
    }
    onstack (this_type&& o) : traits_(o.traits_)
    {
        if (traits_)
            traits_->construct(storage_.address(), std::move(*o.get()));
    }
    this_type& operator=(this_type const& o)
    {
        /**/ if (!traits_ && !o.traits_);
        else if ( traits_ &&  o.traits_) traits_->assign(get(), *o.get());
        else if ( traits_ && !o.traits_) traits_->destroy(get());
        else if (!traits_ &&  o.traits_) o.traits_->construct(storage_.address(), *o.get());

        traits_ = o.traits_;

        return *this;
    }
    this_type& operator=(this_type&& o)
    {
        /**/ if (!traits_ && !o.traits_);
        else if ( traits_ &&  o.traits_) traits_->assign(get(), std::move(*o.get()));
        else if ( traits_ && !o.traits_) traits_->destroy(get());
        else if (!traits_ &&  o.traits_) o.traits_->construct(storage_.address(), std::move(*o.get()));

        traits_ = o.traits_;

        return *this;
    }

    template<typename... arg_types>
    onstack(detail::in_place_type, arg_types&&... args)
    {
        emplace<impl_type>(std::forward<arg_types>(args)...);
    }

    template<typename derived_type, typename... arg_types>
    void
    emplace(arg_types&&... args)
    {
        static_assert(sizeof(derived_type) <= sizeof(storage_type),
                "Attempting to construct type larger than storage area");
        static_assert((alignof(storage_type) % alignof(derived_type)) == 0,
                "Attempting to construct type in storage area that does not have an integer multiple of the type's alignment requirement.");

        ::new (storage_.address()) derived_type(std::forward<arg_types>(args)...);
        traits_ = traits_type();
    }
    impl_type* get () const { return traits_ ? (impl_type*) storage_.address() : nullptr; }

    private:

    storage_type storage_;
    traits_ptr    traits_ = nullptr;
};

#endif // IMPL_PTR_DETAIL_ONSTACK_HPP
