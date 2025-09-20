#ifndef ECRYPTER_HPP
#define ECRYPTER_HPP

#include <type_traits>

namespace ecry
{
    template<class _Ty>
    using clean_type = typename std::remove_const_t<std::remove_reference_t<_Ty>>;

    template <int size_, char key1_, char key2_, typename T>
    class ecrypter
    {
    public:
        __forceinline constexpr ecrypter( T* data )
        {
            crypt( data );
        }

        __forceinline T* get( )
        {
            return storage_;
        }

        __forceinline int size( )
        {
            return size_;
        }

        __forceinline char key( )
        {
            return key1_;
        }

        __forceinline T* encrypt( )
        {
            if ( !is_encrypted( ) )
            {
                this->crypt( storage_ );
            }

            return storage_;
        }

        __forceinline T* decrypt( )
        {
            if ( this->is_encrypted( ) )
            {
                this->crypt( storage_ );
            }

            return storage_;
        }

        __forceinline bool is_encrypted( )
        {
            return storage_[ size_ - 1 ] != 0;
        }

        __forceinline void clear( )
        {
            for ( int i = 0; i < size_; i++ )
            {
                storage_[ i ] = 0;
            }
        }

        __forceinline operator T* ( )
        {
            this->decrypt( );

            return storage_;
        }

    private:
        __forceinline constexpr void crypt( T* data )
        {
            for ( int i = 0; i < size_; i++ )
            {
                storage_[ i ] = data[ i ] ^ ( key1_ + i % ( 1 + key2_ ) );
            }
        }

        T storage_[ size_ ]{};
    };
}

#define ecrypt(str) ecrypt_key(str, __TIME__[4], __TIME__[7]).decrypt()
#define ecrypt_key(str, key1, key2) []() { \
            constexpr static auto crypted = ecry::ecrypter \
                <sizeof(str) / sizeof(str[0]), key1, key2, ecry::clean_type<decltype(str[0])>>((ecry::clean_type<decltype(str[0])>*)str); \
                    return crypted; }()

#endif // !ECRYPTER_HPP