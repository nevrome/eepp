#ifndef EECTEXTUREFONT_H
#define EECTEXTUREFONT_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/graphics/cfont.hpp>

namespace EE { namespace Graphics {

/** @brief This class loads texture fonts and draw strings to the screen. */
class EE_API cTextureFont : public cFont {
	public:
		cTextureFont( const std::string FontName );

		/** The destructor will not unload the texture from memory. If you want that you'll have to remove it manually ( cTextureFactory::instance()->Remove( MyFontInstance->GetTexId() ) ). */
		virtual ~cTextureFont();

		/** Load's a texture font
		* @param TexId The texture id returned by cTextureFactory
		* @param StartChar The fist char represented on the texture
		* @param Spacing The space between every char ( default 0 means TextureWidth / TexColumns )
		* @param VerticalDraw If true render the string verticaly
		* @param TexColumns The number of chars per column
		* @param TexRows The number of chars per row
		* @param NumChars The number of characters to read from the texture
		* @return True if success
		*/
		bool Load( const Uint32& TexId, const eeUint& StartChar = 0, const eeUint& Spacing = 0, const bool& VerticalDraw = false, const eeUint& TexColumns = 16, const eeUint& TexRows = 16, const Uint16& NumChars = 256 );

		/** Load's a texture font and then load's the character coordinates file ( generated by the cTTFFont class )
		* @param TexId The texture id returned by cTextureFactory
		* @param CoordinatesDatPath The character coordinates file
		* @param VerticalDraw If true render the string verticaly
		* @return True if success
		*/
		bool Load( const Uint32& TexId, const std::string& CoordinatesDatPath, const bool& VerticalDraw = false );

		/**
		* @param TexId The texture id returned by cTextureFactory
		* @param Pack Pointer to the pack instance
		* @param FilePackPath The path of the file inside the pack
		* @param VerticalDraw If true render the string verticaly
		* @return True success
		*/
		bool LoadFromPack( const Uint32& TexId, cPack * Pack, const std::string& FilePackPath, const bool& VerticalDraw = false );

		/** Load's a texture font and then load's the character coordinates file previously loaded on memory ( generated by the cTTFFont class )
		* @param TexId The texture id returned by cTextureFactory
		* @param CoordinatesDatPath The character coordinates file
		* @param CoordDataSize The size of CoordData
		* @param VerticalDraw If true render the string verticaly
		* @return True if success
		*/
		bool LoadFromMemory( const Uint32& TexId, const char* CoordData, const Uint32& CoordDataSize, const bool& VerticalDraw = false );

		/** Load's a texture font and then load's the character coordinates from a IO stream file ( generated by the cTTFFont class )
		* @param TexId The texture id returned by cTextureFactory
		* @param IOS IO stream file for the coordinates
		* @param VerticalDraw If true render the string verticaly
		* @return True if success
		*/
		bool LoadFromStream( const Uint32& TexId, cIOStream& IOS, const bool& VerticalDraw );
	private:
		eeUint mStartChar;
		eeUint mTexColumns;
		eeUint mTexRows;
		eeUint mSpacing;
		eeUint mNumChars;

		eeFloat mtX;
		eeFloat mtY;
		eeFloat mFWidth;
		eeFloat mFHeight;

		bool mLoadedCoords;

		void BuildFont();

		void BuildFromGlyphs();
};

}}

#endif
