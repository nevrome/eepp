#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/core/string.hpp>

namespace EE { namespace Graphics {

Float PixelDensity::sPixelDensity = 1.f;

Float PixelDensity::toFloat( EE_PIXEL_DENSITY pd ) {
	switch ( pd )
	{
		case PD_MDPI: return 1.f;
		case PD_HDPI: return 1.5f;
		case PD_XHDPI: return 2.f;
		case PD_XXHDPI: return 3.f;
		case PD_XXXHDPI: return 4.f;
		default: return 1.f;
	}
}

Float PixelDensity::toFloat( Uint32 pd ) {
	return toFloat( (EE_PIXEL_DENSITY)pd );
}

EE_PIXEL_DENSITY PixelDensity::fromString( std::string str ) {
	String::toLowerInPlace( str );
	if ( "mdpi" == str )			return PD_MDPI;
	else if ( "hdpi" == str )		return PD_HDPI;
	else if ( "xhdpi" == str )		return PD_XHDPI;
	else if ( "xxhdpi" == str )		return PD_XXHDPI;
	else if ( "xxxhdpi" == str )	return PD_XXXHDPI;
	return PD_MDPI;
}

EE_PIXEL_DENSITY PixelDensity::fromString( String str ) {
	return fromString( str.toUtf8() );
}

EE_PIXEL_DENSITY PixelDensity::fromDPI( Float dpi ) {
	EE_PIXEL_DENSITY pd = PD_MDPI;

	if ( dpi > 105 && dpi < 160 ) {
		pd = PD_HDPI;
	} else if ( dpi >= 160 && dpi < 240 ) {
		pd = PD_XHDPI;
	} else if ( dpi > 240 && dpi < 320 ) {
		pd = PD_XXHDPI;
	} else if ( dpi >= 320 ) {
		pd = PD_XXXHDPI;
	}

	return pd;
}

const Float& PixelDensity::getPixelDensity() {
	return sPixelDensity;
}

void PixelDensity::setPixelDensity( const Float& pixelDensity ) {
	sPixelDensity = pixelDensity;
}

void PixelDensity::setPixelDensity( const EE_PIXEL_DENSITY& pixelDensity ) {
	sPixelDensity = toFloat( pixelDensity );
}

Float PixelDensity::dpToPx( Float dp ) {
	return dp * getPixelDensity();
}

Int32 PixelDensity::dpToPxI( Float dp ) {
	return (Int32)dpToPx( dp );
}

Float PixelDensity::pxToDp( Float px ) {
	return px / getPixelDensity();
}

Int32 PixelDensity::pxToDpI( Float px ) {
	return (Int32)pxToDp( px );
}

Sizei PixelDensity::dpToPxI( Sizei size ) {
	return Sizei( dpToPxI( size.x ), dpToPxI( size.y ) );
}

Sizei PixelDensity::pxToDpI( Sizei size ) {
	return Sizei( pxToDpI( size.x ), pxToDpI( size.y ) );
}

Rect PixelDensity::dpToPxI( Rect rect ) {
	return rect * getPixelDensity();
}

Rect PixelDensity::pxToDpI( Rect rect ) {
	return rect / getPixelDensity();
}

Rectf PixelDensity::dpToPx( Rectf rect ) {
	return rect * getPixelDensity();
}

Rectf PixelDensity::pxToDp( Rectf rect ) {
	return rect / getPixelDensity();
}

Sizef PixelDensity::dpToPx( Sizef size ) {
	return size * getPixelDensity();
}

Sizef PixelDensity::pxToDp( Sizef size ) {
	return size / getPixelDensity();
}

Sizei PixelDensity::dpToPxI( Sizef size ) {
	return Sizei( dpToPxI( size.x ), dpToPxI( size.y ) );
}

Sizei PixelDensity::pxToDpI( Sizef size ) {
	return Sizei( pxToDpI( size.x ), pxToDpI( size.y ) );
}

Vector2i PixelDensity::dpToPxI( Vector2i pos ) {
	return Sizei( dpToPxI( pos.x ), dpToPxI( pos.y ) );
}

Vector2i PixelDensity::pxToDpI( Vector2i pos ) {
	return Sizei( pxToDpI( pos.x ), pxToDpI( pos.y ) );
}

Vector2f PixelDensity::dpToPx( Vector2f pos ) {
	return Vector2f( dpToPx( pos.x ), dpToPx( pos.y ) );
}

Vector2f PixelDensity::pxToDp( Vector2f pos ) {
	return Vector2f( pxToDp( pos.x ), pxToDp( pos.y ) );
}

Float PixelDensity::toDpFromString( const std::string& str ) {
	std::string num;
	std::string unit;

	for ( std::size_t i = 0; i < str.size(); i++ ) {
		if ( String::isNumber( str[i], true ) || ( '-' == str[i] && i == 0 ) || ( '+' == str[i] && i == 0 ) ) {
			num += str[i];
		} else {
			unit = str.substr( i );
			break;
		}
	}

	if ( !num.empty() ) {
		Float val = 0;
		bool res = String::fromString<Float>( val, num );

		if ( res ) {
			if ( unit == "dp" || unit == "dip" ) {
				return val;
			} else if ( unit == "px" ) {
				return pxToDp( val );
			} else {
				return val;
			}
		}
	}

	return 0;
}

Float PixelDensity::toDpFromStringI( const std::string& str ) {
	return (Int32)toDpFromString( str );
}

}}
