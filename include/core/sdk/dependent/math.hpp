#ifndef MATH_HPP
#define MATH_HPP

namespace math {

	class vector3;
	class vector2;

	struct quaternion;

	struct matrix4x4;
	struct matrix3x4;

	class vector3
	{
	public:
		float x = 0.0f, y = 0.0f, z = 0.0f;

		vector3( ) = default;
		vector3( float _x, float _y, float _z ) : x( _x ), y( _y ), z( _z ) { }

		vector3 operator+( const vector3& v ) const { return { x + v.x, y + v.y, z + v.z }; }
		vector3 operator-( const vector3& v ) const { return { x - v.x, y - v.y, z - v.z }; }
		vector3 operator*( float scalar ) const { return { x * scalar, y * scalar, z * scalar }; }
		vector3 operator/( float scalar ) const { return { x / scalar, y / scalar, z / scalar }; }

		vector3 operator-( ) const { return { -x, -y, -z }; }

		vector3& operator*=( float scalar ) { x *= scalar; y *= scalar; z *= scalar; return *this; }
		vector3& operator/=( float scalar ) { x /= scalar; y /= scalar; z /= scalar; return *this; }
		vector3& operator+=( const vector3& v ) { x += v.x; y += v.y; z += v.z; return *this; }
		vector3& operator-=( const vector3& v ) { x -= v.x; y -= v.y; z -= v.z; return *this; }

		bool operator==( const vector3& v ) const { return x == v.x && y == v.y && z == v.z; }
		bool operator!=( const vector3& v ) const { return !( *this == v ); }

		float dot( const vector3& v ) const { return x * v.x + y * v.y + z * v.z; }
		float distance( const vector3& v ) const { return ( *this - v ).length( ); }
		float length( ) const { return std::sqrt( x * x + y * y + z * z ); }

		vector3 cross( const vector3& v ) const
		{
			return { y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x };
		}

		vector3 normalized( ) const
		{
			const auto len = this->length( );
			if ( len == 0.0f )
			{
				return { 0.0f, 0.0f, 0.0f };
			}

			return *this / len;
		}

		vector3 transform( const matrix3x4& m ) const;
	};

	class vector2
	{
	public:
		float x = 0.0f, y = 0.0f;

		vector2( ) = default;
		vector2( float _x, float _y ) : x( _x ), y( _y ) { }

		vector2 operator+( const vector2& v ) const { return { x + v.x, y + v.y }; }
		vector2 operator-( const vector2& v ) const { return { x - v.x, y - v.y }; }
		vector2 operator*( float scalar ) const { return { x * scalar, y * scalar }; }
		vector2 operator/( float scalar ) const { return { x / scalar, y / scalar }; }

		vector2& operator/=( float scalar ) { x /= scalar; y /= scalar; return *this; }
		vector2& operator*=( float scalar ) { x *= scalar; y *= scalar; return *this; }
		vector2& operator+=( const vector2& v ) { x += v.x; y += v.y; return *this; }
		vector2& operator-=( const vector2& v ) { x -= v.x; y -= v.y; return *this; }

		bool operator==( const vector2& v ) const { return x == v.x && y == v.y; }
		bool operator!=( const vector2& v ) const { return !( *this == v ); }
		bool operator<( const vector2& v ) const { if ( x < v.x ) return true; if ( x > v.x ) return false; return y < v.y; }

		float dot( const vector2& v ) const { return x * v.x + y * v.y; }
		float distance( const vector2& v ) const { return std::sqrt( ( v.x - x ) * ( v.x - x ) + ( v.y - y ) * ( v.y - y ) ); }
		float length( ) const { return std::sqrt( x * x + y * y ); }

		float cross( const math::vector2& rhs ) const
		{
			return x * rhs.y - y * rhs.x;
		}

		vector2 normalized( ) const
		{
			const auto len = this->length( );
			if ( len == 0.0f )
			{
				return { 0.0f, 0.0f };
			}
			return { x / len, y / len };
		}

		ImVec2 vec( ) const
		{
			return ImVec2( x, y );
		}
	};

	struct quaternion
	{
		quaternion( ) : x( 0.0f ), y( 0.0f ), z( 0.0f ), w( 1.f ) { }

		float x, y, z, w;

		vector3 rotate_vector( const vector3& v ) const
		{
			const vector3 qvec( x, y, z );
			vector3 uv = qvec.cross( v );
			vector3 uuv = qvec.cross( uv );
			uv *= ( 2.0f * w );
			uuv *= 2.0f;

			return v + uv + uuv;
		}
	};

	struct transform
	{
		math::vector3 position;
		std::uint8_t pad[ 4 ];
		math::quaternion orientation;
	};

	struct matrix4x4
	{
		float matrix[ 4 ][ 4 ];

		float* operator[]( int index )
		{
			return matrix[ index ];
		}

		const float* operator[]( int index ) const
		{
			return matrix[ index ];
		}

		bool is_zero( float epsilon = 1e-6f ) const
		{
			for ( int row = 0; row < 4; ++row )
			{
				for ( int col = 0; col < 4; ++col )
				{
					if ( std::fabs( matrix[ row ][ col ] ) > epsilon )
					{
						return false;
					}
				}
			}

			return true;
		}
	};

	struct matrix3x4
	{
		float mat[ 3 ][ 4 ];

		const float* operator[]( int i ) const { return mat[ i ]; }
		float* operator[]( int i ) { return mat[ i ]; }
	};

	namespace func {

		inline vector2 from_angle( float angle, float length = 1.0f )
		{
			return { std::cos( angle ) * length, std::sin( angle ) * length };
		}

		inline quaternion from_euler_angles( const vector3& angles_deg )
		{
			float cy = std::cos( std::numbers::pi_v<float> *angles_deg.y * 0.5f / 180.0f );
			float sy = std::sin( std::numbers::pi_v<float> *angles_deg.y * 0.5f / 180.0f );
			float cp = std::cos( std::numbers::pi_v<float> *angles_deg.x * 0.5f / 180.0f );
			float sp = std::sin( std::numbers::pi_v<float> *angles_deg.x * 0.5f / 180.0f );
			float cr = std::cos( std::numbers::pi_v<float> *angles_deg.z * 0.5f / 180.0f );
			float sr = std::sin( std::numbers::pi_v<float> *angles_deg.z * 0.5f / 180.0f );

			quaternion q;
			q.w = cy * cp * cr + sy * sp * sr;
			q.x = cy * sp * cr + sy * cp * sr;
			q.y = sy * cp * cr - cy * sp * sr;
			q.z = cy * cp * sr - sy * sp * cr;
			return q;
		}

		inline vector3 quaternion_to_euler( const quaternion& q )
		{
			vector3 angles;

			const float sinp = 2.f * ( q.w * q.x + q.y * q.z );
			const float cosp = 1.f - 2.f * ( q.x * q.x + q.y * q.y );
			angles.x = std::atan2( sinp, cosp ) * ( 180.0f / std::numbers::pi_v<float> );

			const float siny = 2.f * ( q.w * q.y - q.z * q.x );
			if ( std::abs( siny ) >= 1.f )
			{
				angles.y = std::copysign( 90.0f, siny );
			}
			else
			{
				angles.y = std::asin( siny ) * ( 180.0f / std::numbers::pi_v<float> );
			}

			const float sinr = 2.f * ( q.w * q.z + q.x * q.y );
			const float cosr = 1.f - 2.f * ( q.y * q.y + q.z * q.z );
			angles.z = std::atan2( sinr, cosr ) * ( 180.0f / std::numbers::pi_v<float> );

			return angles;
		}

		inline matrix3x4 transform_to_matrix3x4( const transform& t )
		{
			matrix3x4 matrix{};

			const auto& q = t.orientation;
			const auto& pos = t.position;

			const float xx = q.x * q.x;
			const float yy = q.y * q.y;
			const float zz = q.z * q.z;
			const float xy = q.x * q.y;
			const float xz = q.x * q.z;
			const float yz = q.y * q.z;
			const float wx = q.w * q.x;
			const float wy = q.w * q.y;
			const float wz = q.w * q.z;

			matrix[ 0 ][ 0 ] = 1.0f - 2.0f * ( yy + zz );
			matrix[ 0 ][ 1 ] = 2.0f * ( xy - wz );
			matrix[ 0 ][ 2 ] = 2.0f * ( xz + wy );
			matrix[ 0 ][ 3 ] = pos.x;

			matrix[ 1 ][ 0 ] = 2.0f * ( xy + wz );
			matrix[ 1 ][ 1 ] = 1.0f - 2.0f * ( xx + zz );
			matrix[ 1 ][ 2 ] = 2.0f * ( yz - wx );
			matrix[ 1 ][ 3 ] = pos.y;

			matrix[ 2 ][ 0 ] = 2.0f * ( xz - wy );
			matrix[ 2 ][ 1 ] = 2.0f * ( yz + wx );
			matrix[ 2 ][ 2 ] = 1.0f - 2.0f * ( xx + yy );
			matrix[ 2 ][ 3 ] = pos.z;

			return matrix;
		}

	} // namespace func

	inline math::vector3 math::vector3::transform( const math::matrix3x4& m ) const
	{
		return {
			x * m[ 0 ][ 0 ] + y * m[ 0 ][ 1 ] + z * m[ 0 ][ 2 ] + m[ 0 ][ 3 ],
			x * m[ 1 ][ 0 ] + y * m[ 1 ][ 1 ] + z * m[ 1 ][ 2 ] + m[ 1 ][ 3 ],
			x * m[ 2 ][ 0 ] + y * m[ 2 ][ 1 ] + z * m[ 2 ][ 2 ] + m[ 2 ][ 3 ]
		};
	}

} // namespace math

#endif // !MATH_HPP
