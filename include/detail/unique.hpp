#ifndef AUXILIARY_PIMPL_DETAIL_UNIQUE_HPP
#define AUXILIARY_PIMPL_DETAIL_UNIQUE_HPP

#include "./traits.hpp"
#include "./is_allocator.hpp"

namespace detail
{
    template<typename> struct unique;
}

template<typename impl_type>
struct detail::unique
{
    // Smart-pointer with the value-semantics behavior.
    // The incomplete-type management technique is originally by Peter Dimov.

    using        this_type = unique;
    using base_traits_type = detail::traits::base<impl_type>;
    using copy_traits_type = detail::traits::deep_copy<impl_type>;

//  template<typename alloc_type, typename... arg_types>
//  typename std::enable_if<is_allocator<alloc_type>::value, void>::type
//  construct(alloc_type&& alloc, arg_types&&... args)
//  {
//      void* mem = std::allocator_traits<alloc_type>::allocate(alloc, 1);
//      reset(new(mem) impl_type(std::forward<arg_types>(args)...));
//  }
    template<typename... arg_types>
    typename std::enable_if<!is_allocator<typename first<arg_types...>::type>::value, void>::type
    construct(arg_types&&... args)
    {
        reset(new impl_type(std::forward<arg_types>(args)...));
    }

   ~unique () { if (traits_) traits_->destroy(impl_); }
    unique () {}
    unique (impl_type* p) : impl_(p), traits_(copy_traits_type()) {}
    unique (this_type&& o) { swap(o); }
    unique (this_type const& o)
    :
        impl_(o.traits_ ? o.traits_->copy(o.impl_) : nullptr),
        traits_(o.traits_)
    {}

    this_type& operator= (this_type&& o) { swap(o); return *this; }
    this_type& operator= (this_type const& o) { traits_ = o.traits_; traits_->assign(impl_, o.impl_); return *this; }
    bool       operator< (this_type const& o) const { return impl_ < o.impl_; }

    void     reset (impl_type* p) { this_type(p).swap(*this); }
    void      swap (this_type& o) { std::swap(impl_, o.impl_), std::swap(traits_, o.traits_); }
    impl_type* get () const { return impl_; }
    long use_count () const { return 1; }

    private:

    impl_type*                impl_ = nullptr;
    base_traits_type const* traits_ = nullptr;
};

#endif // AUXILIARY_PIMPL_DETAIL_UNIQUE_HPP