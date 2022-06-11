
//          Copyright Ted Middleton 2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef INCLUDED_mainframe_frame_iterator_h
#define INCLUDED_mainframe_frame_iterator_h

#include <iterator>
#include "mainframe/series.hpp"

namespace mf
{

template< typename ... Ts >
struct pointerize;

template< typename ... Ts >
struct pointerize<std::tuple< series<Ts>... >>
{
    using series_tuple_type = std::tuple< series<Ts>... >;
    using type = std::tuple< Ts*... >;
    using const_type = std::tuple< Ts*... >;

    static type op( series_tuple_type& sertup, ptrdiff_t offset = 0 )
    {
        type out;
        assign_impl<0>( sertup, offset, out );
        return out;
    }

    template< size_t Ind >
    static void assign_impl( series_tuple_type& sertup, 
                             ptrdiff_t offset, 
                             type& out )
    {
        using T = typename detail::pack_element<Ind, Ts...>::type;
        series<T>& sertupelem = std::get<Ind>( sertup );
        std::get<Ind>( out ) = sertupelem.data() + offset;
        if constexpr ( Ind+1 < sizeof...( Ts ) ) {
            assign_impl< Ind+1 >( sertup, offset, out );
        }
    }

};

template< size_t Ind >
struct expr_column;

template< typename T >
struct terminal;

template< size_t Ind >
using columnindex = terminal<expr_column<Ind>>;

template< typename ... Ts >
class _row_proxy;

template< typename ... Ts >
class frame_row
{
public:
    using row_type = std::true_type;

    frame_row( const std::tuple<Ts...>& args ) 
        : data( args ) 
    {}

    frame_row( std::tuple<Ts...>&& args ) 
        : data( std::move( args ) ) 
    {}

    frame_row( const frame_row& other )
        : data( other.data )
    {}

    frame_row( frame_row&& other )
        : data( std::move( other.data ) )
    {}

    frame_row( const _row_proxy< Ts... >& refs )
    {
        init<0>( refs );
    }

    frame_row& operator=( const frame_row& other )
    {
        init<0>( other );
        return *this;
    }

    frame_row& operator=( frame_row&& other )
    {
        init<0>( std::move( other ) );
        return *this;
    }

    frame_row& operator=( const _row_proxy< Ts... >& refs )
    {
        init<0>( refs );
        return *this;
    }

    template< size_t Ind >
    typename detail::pack_element<Ind, Ts...>::type&
    at() const
    {
        return std::get<Ind>( data );
    }

    template< size_t Ind >
    typename detail::pack_element<Ind, Ts...>::type&
    at( columnindex<Ind> ) const
    {
        return std::get<Ind>( data );
    }

private:

    template< size_t Ind, template < typename... > typename Row >
    void init( const Row< Ts... >& refs )
    {
        columnindex<Ind> ci;
        at( ci ) = refs.at( ci );
        if constexpr ( Ind+1 < sizeof...( Ts ) ) {
            init<Ind+1>( refs );
        }
    }

    template< size_t Ind, template < typename... > typename Row >
    void init( Row< Ts... >&& refs )
    {
        columnindex<Ind> ci;
        at( ci ) = std::move( refs.at( ci ) );
        if constexpr ( Ind+1 < sizeof...( Ts ) ) {
            init<Ind+1>( std::move( refs ) );
        }
    }

    std::tuple<Ts...> data;
};

template< typename ... Ts >
class _row_proxy
{
public:
    using row_type = std::true_type;

    _row_proxy( std::tuple< Ts*... > p ) 
        : m_ptrs( p ) 
    {}

    _row_proxy& operator=( const _row_proxy& row )
    {
        init<0>( row );
        return *this;
    }

    // by-value swap
    template< size_t Ind = 0 >
    void swap_values( _row_proxy& other ) noexcept
    {
        using std::swap;
        columnindex<Ind> ci;
        swap( at( ci ), other.at( ci ) );
        if constexpr ( Ind + 1 < sizeof...(Ts) ) {
            swap_values< Ind + 1 >( other );
        }
    }

    template< size_t Ind >
    typename detail::pack_element< Ind, Ts...>::type&
    at() const
    {
        return *std::get<Ind>( m_ptrs );
    }

    template< size_t Ind >
    typename detail::pack_element< Ind, Ts...>::type&
    at( columnindex<Ind> ) const
    {
        return *std::get<Ind>( m_ptrs );
    }

    template< size_t Ind >
    typename detail::pack_element< Ind, Ts...>::type&
    operator[]( columnindex<Ind> ) const
    {
        return *std::get<Ind>( m_ptrs );
    }

    template< size_t Ind >
    typename detail::pack_element< Ind, Ts...>::type*
    data( columnindex<Ind> ) const
    {
        return std::get<Ind>( m_ptrs );
    }

    bool any_missing() const
    {
        return any_missing_impl<0>();
    }

    template< size_t Ind=0 >
    bool any_missing_impl() const
    {
        using T = typename detail::pack_element<Ind, Ts...>::type;
        const T& elem = at< Ind >();
        if constexpr ( detail::is_missing<T>::value ) {
            if ( !elem.has_value() ) {
                return true;
            }
        }
        if constexpr ( Ind+1 < sizeof...(Ts) ) {
            return any_missing_impl<Ind+1>();
        }
        return false;
    }

private:

    template< bool IsConst, typename ... Us >
    friend class base_frame_iterator;

    // Only base_frame_iterator should be able to create one of these
    _row_proxy( const _row_proxy& other )
        : m_ptrs( other.m_ptrs )
    {
    }
    
    void swap_ptrs( _row_proxy& other ) noexcept
    {
        swap( m_ptrs, other.m_ptrs );
    }

    ptrdiff_t addr_diff( const _row_proxy& other ) const
    {
        using T = typename detail::pack_element<0, Ts...>::type;
        const T* ptr = std::get<0>( m_ptrs );
        const T* otherptr = std::get<0>( other.m_ptrs );
        return ptr - otherptr;
    }

    bool addr_eq( const _row_proxy& other ) const
    {
        return std::get<0>( m_ptrs ) == std::get<0>( other.m_ptrs );
    }

    bool addr_ne( const _row_proxy& other ) const
    {
        return std::get<0>( m_ptrs ) != std::get<0>( other.m_ptrs );
    }

    bool addr_le( const _row_proxy& other ) const
    {
        return std::get<0>( m_ptrs ) <= std::get<0>( other.m_ptrs );
    }

    bool addr_lt( const _row_proxy& other ) const
    {
        return std::get<0>( m_ptrs ) < std::get<0>( other.m_ptrs );
    }

    bool addr_ge( const _row_proxy& other ) const
    {
        return std::get<0>( m_ptrs ) >= std::get<0>( other.m_ptrs );
    }

    bool addr_gt( const _row_proxy& other ) const
    {
        return std::get<0>( m_ptrs ) > std::get<0>( other.m_ptrs );
    }

    template< size_t Ind = 0 >
    void addr_modify( ptrdiff_t n )
    {
        std::get< Ind >( m_ptrs ) += n;
        if constexpr ( Ind+1 < sizeof...( Ts ) ) {
            addr_modify< Ind+1 >( n );
        }
    }

    _row_proxy new_row_plus_offset( ptrdiff_t off ) const
    {
        _row_proxy out( m_ptrs );
        out.addr_modify( off );
        return out;
    }

    template< size_t Ind, template <typename...> typename Row >
    void init( const Row<Ts...>& vals )
    {
        columnindex<Ind> ci;
        at( ci ) = vals.at( ci );
        if constexpr ( Ind+1 < sizeof...( Ts ) ) {
            init<Ind+1>( vals );
        }
    }

    std::tuple< Ts*... > m_ptrs;
};

template< typename ... Ts >
void swap( mf::_row_proxy< Ts... >& left, mf::_row_proxy< Ts... >& right ) noexcept
{
    left.swap_values( right );
}

template< size_t Ind=0, typename ... Ts >
bool operator==( const _row_proxy< Ts... >& left, 
                 const _row_proxy< Ts... >& right )
{
    columnindex<Ind> ci;
    const auto& leftval = left.at( ci );
    const auto& rightval = right.at( ci );
    if ( leftval == rightval ) {
        if constexpr ( Ind+1 < sizeof...( Ts ) ) {
            return operator==<Ind+1>( left, right );
        }
        else {
            return true;
        }
    }
    else {
        return false;
    }
}

template< size_t Ind=0, typename ... Ts >
bool operator!=( const _row_proxy< Ts... >& left, 
                 const _row_proxy< Ts... >& right )
{
    return !(left == right);
}

template< size_t Ind=0, typename ... Ts >
bool operator<=( const _row_proxy< Ts... >& left, 
                 const _row_proxy< Ts... >& right )
{
    columnindex<Ind> ci;
    const auto& leftval = left.at( ci );
    const auto& rightval = right.at( ci );
    if ( leftval < rightval ) {
        return true;
    }
    else if ( leftval == rightval ) {
        if constexpr ( Ind+1 < sizeof...( Ts ) ) {
            return operator<= <Ind+1>( left, right );
        }
        else {
            return true;
        }
    }
    else {
        return false;
    }
}

template< size_t Ind=0, typename ... Ts >
bool operator>=( const _row_proxy< Ts... >& left, 
                 const _row_proxy< Ts... >& right )
{
    columnindex<Ind> ci;
    const auto& leftval = left.at( ci );
    const auto& rightval = right.at( ci );
    if ( leftval > rightval ) {
        return true;
    }
    else if ( leftval == rightval ) {
        if constexpr ( Ind+1 < sizeof...( Ts ) ) {
            return operator>= <Ind+1>( left, right );
        }
        else {
            return true;
        }
    }
    else {
        return false;
    }
}

template< size_t Ind=0, typename ... Ts >
bool operator<(  const _row_proxy< Ts... >& left, 
                 const _row_proxy< Ts... >& right )
{
    columnindex<Ind> ci;
    const auto& leftval = left.at( ci );
    const auto& rightval = right.at( ci );
    if ( leftval < rightval ) {
        return true;
    }
    else if ( leftval == rightval ) {
        if constexpr ( Ind+1 < sizeof...( Ts ) ) {
            return operator< <Ind+1>( left, right );
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
}

template< size_t Ind=0, typename ... Ts >
bool operator>(  const _row_proxy< Ts... >& left, 
                 const _row_proxy< Ts... >& right )
{
    columnindex<Ind> ci;
    const auto& leftval = left.at( ci );
    const auto& rightval = right.at( ci );
    if ( leftval > rightval ) {
        return true;
    }
    else if ( leftval == rightval ) {
        if constexpr ( Ind+1 < sizeof...( Ts ) ) {
            return operator> <Ind+1>( left, right );
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
}

template< bool IsConst, typename ... Ts >
class base_frame_iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = ptrdiff_t;
    using value_type = frame_row<Ts...>;
    using reference = _row_proxy<Ts...>&;
    using pointer = _row_proxy<Ts...>*;

    base_frame_iterator() = default;

    base_frame_iterator( const std::tuple<series<Ts>...>& data, difference_type off = 0 )
        : m_row( pointerize<std::tuple<series<Ts>...>>::op( const_cast< std::tuple<series<Ts>...>&>(data), off ) )
    {}

    base_frame_iterator( std::tuple<Ts*...> ptrs )
        : m_row( ptrs )
    {}

    base_frame_iterator( _row_proxy<Ts...> r )
        : m_row( r )
    {}

    void operator++() { operator+=( 1 ); }
    void operator++(int) { operator+=( 1 ); }
    void operator--() { operator-=( 1 ); }
    void operator--(int) { operator-=( 1 ); }

    void operator+=( ptrdiff_t n )
    {
        m_row.addr_modify( n );
    }
    void operator-=( ptrdiff_t n )
    {
        m_row.addr_modify( -n );
    }

    reference operator*() const { return const_cast<reference>(m_row); }
    pointer operator->() const { return const_cast<pointer>(&m_row); }

    template< size_t Ind >
    base_sv_iterator<typename detail::pack_element< Ind, Ts... >::type>
    column_iterator( columnindex< Ind > ci ) const
    {
        using T = typename detail::pack_element< Ind, Ts... >::type;
        base_sv_iterator<T> it{ m_row.data( ci ) };
        return it;
    }

    void swap( base_frame_iterator& other ) noexcept
    {
        m_row.swap_ptrs( other );
    }

    bool operator==( const base_frame_iterator& other ) const
    {
        return m_row.addr_eq( other.m_row );
    }

    bool operator!=( const base_frame_iterator& other ) const
    {
        return m_row.addr_ne( other.m_row );
    }

    bool operator<( const base_frame_iterator& other ) const
    {
        return m_row.addr_lt( other.m_row );
    }

    bool operator>( const base_frame_iterator& other ) const
    {
        return m_row.addr_gt( other.m_row );
    }

    bool operator<=( const base_frame_iterator& other ) const
    {
        return m_row.addr_le( other.m_row );
    }

    bool operator>=( const base_frame_iterator& other ) const
    {
        return m_row.addr_ge( other.m_row );
    }

    difference_type operator-( const base_frame_iterator& other ) const
    {
        return m_row.addr_diff( other.m_row );
    }

    base_frame_iterator operator+( ptrdiff_t off ) const
    {
        return base_frame_iterator( m_row.new_row_plus_offset( off ) );
    }

    base_frame_iterator operator-( ptrdiff_t off ) const
    {
        return base_frame_iterator( m_row.new_row_plus_offset(-off) );
    }

private:

    _row_proxy< Ts... > m_row;
};

template< typename ... Ts >
using frame_iterator = base_frame_iterator< false, Ts... >;

template< typename ... Ts >
using const_frame_iterator = base_frame_iterator< true, Ts... >;

} // namespace mf

#endif // INCLUDED_mainframe_frame_iterator_h

