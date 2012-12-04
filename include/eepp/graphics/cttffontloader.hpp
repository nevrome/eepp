#ifndef EE_GRAPHICSCTTFFONTLOADER
#define EE_GRAPHICSCTTFFONTLOADER

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/cfont.hpp>
#include <eepp/graphics/cttffont.hpp>
#include <eepp/system/cobjectloader.hpp>

namespace EE { namespace Graphics {

#define TTF_LT_PATH 	(1)
#define TTF_LT_MEM 		(2)
#define TTF_LT_PACK 	(3)

class EE_API cTTFFontLoader : public cObjectLoader {
	public:
		cTTFFontLoader( const std::string& FontName, const std::string& Filepath, const eeUint& Size, EE_TTF_FONT_STYLE Style = EE_TTF_STYLE_NORMAL, const bool& VerticalDraw = false, const Uint16& NumCharsToGen = 512, const eeColor& FontColor = eeColor(), const Uint8& OutlineSize = 0, const eeColor& OutlineColor = eeColor(0,0,0), const bool& AddPixelSeparator = true );

		cTTFFontLoader( const std::string& FontName, cPack * Pack, const std::string& FilePackPath, const eeUint& Size, EE_TTF_FONT_STYLE Style = EE_TTF_STYLE_NORMAL, const bool& VerticalDraw = false, const Uint16& NumCharsToGen = 512, const eeColor& FontColor = eeColor(), const Uint8& OutlineSize = 0, const eeColor& OutlineColor = eeColor(0,0,0), const bool& AddPixelSeparator = true );

		cTTFFontLoader( const std::string& FontName, Uint8* TTFData, const eeUint& TTFDataSize, const eeUint& Size, EE_TTF_FONT_STYLE Style = EE_TTF_STYLE_NORMAL, const bool& VerticalDraw = false, const Uint16& NumCharsToGen = 512, const eeColor& FontColor = eeColor(), const Uint8& OutlineSize = 0, const eeColor& OutlineColor = eeColor(0,0,0), const bool& AddPixelSeparator = true );

		~cTTFFontLoader();

		void 				Update();

		void				Unload();

		const std::string&	Id() const;

		cFont *				Font() const;
	protected:
		Uint32				mLoadType; 	// From memory, from path, from pack

		cTTFFont *			mFont;

		std::string			mFontName;
		std::string			mFilepath;
		eeUint				mSize;
		EE_TTF_FONT_STYLE	mStyle;
		bool				mVerticalDraw;
		Uint16				mNumCharsToGen;
		eeColor				mFontColor;
		Uint8				mOutlineSize;
		eeColor				mOutlineColor;
		bool				mAddPixelSeparator;
		cPack *				mPack;
		Uint8 *				mData;
		eeUint				mDataSize;

		void 				Start();

		void				Reset();
	private:
		bool				mFontLoaded;

		void 				LoadFromPath();
		void				LoadFromMemory();
		void				LoadFromPack();
		void 				Create();
};

}}

#endif


