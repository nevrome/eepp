#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

UITooltip::UITooltip( UITooltip::CreateParams& Params, UIControl * TooltipOf ) :
	UIControlAnim( Params ),
	mFontColor( Params.FontColor ),
	mFontShadowColor( Params.FontShadowColor ),
	mAlignOffset( 0.f, 0.f ),
	mPadding( Params.Padding ),
	mTooltipTime( Time::Zero ),
	mTooltipOf( TooltipOf )
{
	mTextCache = eeNew( TextCache, () );
	mTextCache->font( Params.Font );
	mTextCache->color( mFontColor );
	mTextCache->shadowColor( mFontShadowColor );

	if ( NULL == Params.Font ) {
		if ( NULL != UIThemeManager::instance()->DefaultFont() )
			mTextCache->font( UIThemeManager::instance()->DefaultFont() );
		else
			eePRINTL( "UITooltip::UITextBox : Created a UI TextBox without a defined font." );
	}

	AutoPadding();

	if ( Params.ParentCtrl != UIManager::instance()->MainControl() )
		Parent( UIManager::instance()->MainControl() );

	ApplyDefaultTheme();
}

UITooltip::~UITooltip() {
	eeSAFE_DELETE( mTextCache );

	if ( NULL != mTooltipOf && mTooltipOf->IsComplex() ) {
		reinterpret_cast<UIComplexControl*>( mTooltipOf )->TooltipRemove();
	}
}

Uint32 UITooltip::Type() const {
	return UI_TYPE_TOOLTIP;
}

bool UITooltip::IsType( const Uint32& type ) const {
	return UITooltip::Type() == type ? true : UIControlAnim::IsType( type );
}

void UITooltip::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "tooltip" );

	AutoPadding();

	if ( NULL == mTextCache->font() && NULL != Theme->Font() ) {
		mTextCache->font( Theme->Font() );
	}
}

void UITooltip::AutoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = MakePadding( true, true, true, true );
	}
}

void UITooltip::Show() {
	if ( !Visible() || 0 == mAlpha ) {
		ToFront();

		Visible( true );

		if ( UIThemeManager::instance()->DefaultEffectsEnabled() ) {
			StartAlphaAnim( 255.f == mAlpha ? 0.f : mAlpha, 255.f, UIThemeManager::instance()->ControlsFadeInTime() );
		}
	}
}

void UITooltip::Hide() {
	if ( Visible() ) {
		if ( UIThemeManager::instance()->DefaultEffectsEnabled() ) {
			DisableFadeOut( UIThemeManager::instance()->ControlsFadeOutTime() );
		} else {
			Visible( false );
		}
	}
}

void UITooltip::Draw() {
	if ( mVisible && 0.f != mAlpha ) {
		UIControlAnim::Draw();

		if ( mTextCache->getTextWidth() ) {
			mTextCache->flags( Flags() );
			mTextCache->draw( (Float)mScreenPos.x + mAlignOffset.x, (Float)mScreenPos.y + mAlignOffset.y, Vector2f::One, 0.f, Blend() );
		}
	}
}

Graphics::Font * UITooltip::Font() const {
	return mTextCache->font();
}

void UITooltip::Font( Graphics::Font * font ) {
	if ( mTextCache->font() != font ) {
		mTextCache->font( font );
		AutoPadding();
		AutoSize();
		AutoAlign();
		OnFontChanged();
	}
}

const String& UITooltip::Text() {
	return mTextCache->text();
}

void UITooltip::Text( const String& text ) {
	mTextCache->text( text );
	AutoPadding();
	AutoSize();
	AutoAlign();
	OnTextChanged();
}

const ColorA& UITooltip::Color() const {
	return mFontColor;
}

void UITooltip::Color( const ColorA& color ) {
	mFontColor = color;
	Alpha( color.a() );
}

const ColorA& UITooltip::ShadowColor() const {
	return mFontShadowColor;
}

void UITooltip::ShadowColor( const ColorA& color ) {
	mFontShadowColor = color;
	Alpha( color.a() );
	mTextCache->shadowColor( mFontColor );
}

void UITooltip::Alpha( const Float& alpha ) {
	UIControlAnim::Alpha( alpha );
	mFontColor.Alpha = (Uint8)alpha;
	mFontShadowColor.Alpha = (Uint8)alpha;

	mTextCache->color( mFontColor );
}

void UITooltip::AutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		mSize.width( (int)mTextCache->getTextWidth() + mPadding.Left + mPadding.Right );
		mSize.height( (int)mTextCache->getTextHeight() + mPadding.Top + mPadding.Bottom );
	}
}

void UITooltip::AutoAlign() {
	Uint32 Width	= mSize.width()		- mPadding.Left - mPadding.Right;
	Uint32 Height	= mSize.height()	- mPadding.Top	- mPadding.Bottom;

	switch ( FontHAlignGet( Flags() ) ) {
		case UI_HALIGN_CENTER:
			mAlignOffset.x = mPadding.Left + (Float)( (Int32)( Width - mTextCache->getTextWidth() ) / 2 );
			break;
		case UI_HALIGN_RIGHT:
			mAlignOffset.x = ( (Float)Width - (Float)mTextCache->getTextWidth() ) - mPadding.Right;
			break;
		case UI_HALIGN_LEFT:
			mAlignOffset.x = mPadding.Left;
			break;
	}

	switch ( FontVAlignGet( Flags() ) ) {
		case UI_VALIGN_CENTER:
			mAlignOffset.y = mPadding.Top + (Float)( ( (Int32)( Height - mTextCache->getTextHeight() ) ) / 2 );
			break;
		case UI_VALIGN_BOTTOM:
			mAlignOffset.y = ( (Float)Height - (Float)mTextCache->getTextHeight() ) - mPadding.Bottom;
			break;
		case UI_VALIGN_TOP:
			mAlignOffset.y = mPadding.Top;
			break;
	}
}

void UITooltip::OnSizeChange() {
	AutoPadding();
	AutoSize();
	AutoAlign();

	UIControlAnim::OnSizeChange();

	mTextCache->cache();
}

void UITooltip::OnTextChanged() {
	SendCommonEvent( UIEvent::EventOnTextChanged );
}

void UITooltip::OnFontChanged() {
	SendCommonEvent( UIEvent::EventOnFontChanged );
}

void UITooltip::Padding( const Recti& padding ) {
	mPadding = padding;
}

const Recti& UITooltip::Padding() const {
	return mPadding;
}

TextCache * UITooltip::GetTextCache() {
	return mTextCache;
}

Float UITooltip::GetTextWidth() {
	return mTextCache->getTextWidth();
}

Float UITooltip::GetTextHeight() {
	return mTextCache->getTextHeight();
}

const int& UITooltip::GetNumLines() const {
	return mTextCache->getNumLines();
}

const Vector2f& UITooltip::AlignOffset() const {
	return mAlignOffset;
}

void UITooltip::TooltipTime( const Time& Time ) {
	mTooltipTime = Time;
}

void UITooltip::TooltipTimeAdd( const Time& Time ) {
	mTooltipTime += Time;
}

const Time& UITooltip::TooltipTime() const {
	return mTooltipTime;
}

}}
