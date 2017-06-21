// Copyright (c) 2006-2016 Vladimir Batov.
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. See http://www.boost.org/LICENSE_1_0.txt.

#ifndef AUXILIARY_PIMPL_HPP
#define AUXILIARY_PIMPL_HPP

#include "./detail/unique.hpp"

namespace detail
{
    template<typename> struct  shared;
    template<typename> struct     cow; // copy_on_write
    template<typename> struct onstack;

    struct null_type {};
}

template<typename impl_type>
struct detail::onstack
{
    // Proof of concept
    // Need to extract storage size from more_types
    char storage_[32];

    template<typename... arg_types>
    void
    construct(arg_types&&... args)
    {
        new (storage_) impl_type(std::forward<arg_types>(args)...);
    }
    onstack () =default;
};

template<typename impl_type>
struct detail::shared : std::shared_ptr<impl_type>
{
    using this_type = shared;
    using base_type = std::shared_ptr<impl_type>;
    using  base_ref = base_type&;

    template<typename... arg_types>
    typename std::enable_if<is_allocator<typename first<arg_types...>::type>::value, void>::type
    construct(arg_types&&... args)
    {
        base_ref(*this) = std::allocate_shared<impl_type>(std::forward<arg_types>(args)...);
    }
    template<typename... arg_types>
    typename std::enable_if<!is_allocator<typename first<arg_types...>::type>::value, void>::type
    construct(arg_types&&... args)
    {
        base_ref(*this) = std::make_shared<impl_type>(std::forward<arg_types>(args)...);
    }
    template<typename derived_type> void construct(std::shared_ptr<derived_type>&&      p) { base_ref(*this) = p; }
    template<typename derived_type> void construct(std::shared_ptr<derived_type> const& p) { base_ref(*this) = p; }
    template<typename derived_type> void construct(std::shared_ptr<derived_type>&       p) { base_ref(*this) = p; }
};

template<typename user_type>
struct impl_ptr
{
    struct          implementation;
    template<typename> struct base;

    using   unique = base<detail::unique <implementation>>;
    using   shared = base<detail::shared <implementation>>;
    using      cow = base<detail::cow    <implementation>>;
    using  onstack = base<detail::onstack<implementation>>;
    using yes_type = boost::type_traits::yes_type;
    using  no_type = boost::type_traits::no_type;
    using ptr_type = typename std::remove_reference<user_type>::type*;

    template<typename Y>
    static yes_type test (Y*, typename Y::impl_ptr_check_type* =nullptr);
    static no_type  test (...);

    BOOST_STATIC_CONSTANT(bool, value = (1 == sizeof(test(ptr_type(nullptr)))));

    static user_type null()
    {
        using null_type = typename user_type::null_type;
        using base_type = typename user_type::base_type;

        static_assert(impl_ptr<user_type>::value, "");
        static_assert(sizeof(user_type) == sizeof(base_type), "");

        null_type  arg;
        base_type null (arg);

        return *(user_type*) &null;
    }
};

template<typename user_type>
template<typename policy_type>
struct impl_ptr<user_type>::base
{
    using      implementation = typename impl_ptr<user_type>::implementation;
    using impl_ptr_check_type = base;
    using           base_type = base;
    using           null_type = detail::null_type;

    template<typename T> using     rm_ref = typename std::remove_reference<T>::type;
    template<typename T> using is_base_of = typename std::is_base_of<base_type, rm_ref<T>>;
    template<typename T> using is_derived = typename std::enable_if<is_base_of<T>::value, null_type*>::type;

    bool         operator! () const { return !impl_.get(); }
    explicit operator bool () const { return  impl_.get(); }

    // Comparison Operators.
    // base::op==() transfers the comparison to 'impl_'. Consequently,
    // ptr-semantics (shared_ptr-based) pimpls are comparable due to shared_ptr::op==().
    // However, value-semantics (unique-based) pimpls are NOT COMPARABLE BY DEFAULT --
    // the standard value-semantics behavior -- due to NO unique::op==().
    // If a value-semantics class T needs to be comparable, then it has to provide
    // T::op==(T const&) EXPLICITLY as part of its public interface.
    // Trying to call this base::op==() for unique-based impl_ptr will fail to compile (no unique::op==())
    // and will indicate that the user forgot to declare T::operator==(T const&).
    bool operator==(base_type const& that) const { return impl_ == that.impl_; }
    bool operator!=(base_type const& that) const { return impl_ != that.impl_; }
    bool operator< (base_type const& that) const { return impl_  < that.impl_; }

    void  swap (base_type& that) { impl_.swap(that.impl_); }

//    template<class Y>                   void reset (Y* p) { impl_.reset(p); }
//    template<class Y, class D>          void reset (Y* p, D d) { impl_.reset(p, d); }
//    template<class Y, class D, class A> void reset (Y* p, D d, A a) { impl_.reset(p, d, a); }

    template<typename... arg_types>
    void
    reset(arg_types&&... args) { impl_.construct(std::forward<arg_types>(args)...); }

    // Access To the Implementation.
    // 1) These methods are only useful and meaningful in the implementation code,
    //    i.e. where impl_ptr<>::implementation is visible.
    // 2) For better or worse the original deep-constness behavior has been changed
    //    to match std::shared_ptr, etc. to avoid questions, confusion, etc.
    implementation* operator->() const { BOOST_ASSERT(impl_.get()); return  impl_.get(); }
    implementation& operator *() const { BOOST_ASSERT(impl_.get()); return *impl_.get(); }

    long use_count() const { return impl_.use_count(); }

    protected:

    template<typename> friend class impl_ptr;

    base (null_type) {}

    template<class arg_type>
    base(arg_type&& arg, is_derived<arg_type> =nullptr) : impl_(arg.impl_) {}

    template<typename... arg_types>
    base(arg_types&&... args) { impl_.construct(std::forward<arg_types>(args)...); }

    private: policy_type impl_;
};

namespace boost
{
    template<typename user_type> using impl_ptr = ::impl_ptr<user_type>;
}

#endif // AUXILIARY_PIMPL_HPP