#include <eepp/window/backend/null/ccursormanagernull.hpp>
#include <eepp/window/backend/null/ccursornull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

CursorManagerNull::CursorManagerNull( EE::Window::Window * window ) :
	CursorManager( window )
{
}

Cursor * CursorManagerNull::Create( cTexture * tex, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorNull, ( tex, hotspot, name, mWindow ) );
}

Cursor * CursorManagerNull::Create( cImage * img, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorNull, ( img, hotspot, name, mWindow ) );
}

Cursor * CursorManagerNull::Create( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorNull, ( path, hotspot, name, mWindow ) );
}

void CursorManagerNull::Set( Cursor * cursor ) {
}

void CursorManagerNull::Set( EE_SYSTEM_CURSOR syscurid ) {
}

void CursorManagerNull::Show() {
}

void CursorManagerNull::Hide() {
}

void CursorManagerNull::Visible( bool visible ) {
}

void CursorManagerNull::Remove( Cursor * cursor, bool Delete ) {
    CursorManager::Remove( cursor, Delete );
}

void CursorManagerNull::Reload() {
}

}}}}
