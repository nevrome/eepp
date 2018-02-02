#include <eepp/ee.hpp>

EE::Window::Window * win = NULL;
UIMessageBox * MsgBox = NULL;
MapEditor * Editor = NULL;

bool onCloseRequestCallback( EE::Window::Window * w ) {
	if ( NULL != Editor ) {
		MsgBox = UIMessageBox::New( MSGBOX_OKCANCEL, "Do you really want to close the current map?\nAll changes will be lost." );
		MsgBox->addEventListener( Event::MsgBoxConfirmClick, cb::Make1<void, const Event*>( []( const Event * event ) { win->close(); } ) );
		MsgBox->addEventListener( Event::OnClose, cb::Make1<void, const Event*>( []( const Event * event ) { MsgBox = NULL; } ) );
		MsgBox->setTitle( "Close Map?" );
		MsgBox->center();
		MsgBox->show();
		return false;
	} else {
		return true;
	}
}

void mainLoop() {
	win->getInput()->update();

	if ( win->getInput()->isKeyUp( KEY_ESCAPE ) && NULL == MsgBox && onCloseRequestCallback( win ) ) {
		win->close();
	}

	UIManager::instance()->update();

	if ( UIManager::instance()->getMainControl()->invalidated() ) {
		win->clear();

		UIManager::instance()->draw();

		win->display();
	} else {
		Sys::sleep( Milliseconds(8) );
	}
}

EE_MAIN_FUNC int main (int argc, char * argv []) {
	Display * currentDisplay = Engine::instance()->getDisplayManager()->getDisplayIndex(0);
	Float pixelDensity = PixelDensity::toFloat( currentDisplay->getPixelDensity() );
	DisplayMode currentMode = currentDisplay->getCurrentMode();

	Uint32 width = eemin( currentMode.Width, (Uint32)( 1280 * pixelDensity ) );
	Uint32 height = eemin( currentMode.Height, (Uint32)( 720 * pixelDensity ) );

	win = Engine::instance()->createWindow( WindowSettings( width, height, "eepp - Map Editor", WindowStyle::Default, WindowBackend::Default, 32, "assets/icon/ee.png", pixelDensity ), ContextSettings( true, GLv_default, true, 24, 1, 0, false ) );

	if ( win->isOpen() ) {
		win->setCloseRequestCallback( cb::Make1( onCloseRequestCallback ) );

		UIManager::instance()->init( UI_MANAGER_USE_DRAW_INVALIDATION );

		{
			std::string pd;
			if ( PixelDensity::getPixelDensity() >= 1.5f ) pd = "1.5x";
			else if ( PixelDensity::getPixelDensity() >= 2.f ) pd = "2x";

			TextureAtlasLoader tgl( "assets/ui/uitheme" + pd + ".eta" );

			UITheme * theme = UITheme::loadFromTextureAtlas( UIThemeDefault::New( "uitheme" + pd, "uitheme" + pd ), TextureAtlasManager::instance()->getByName( "uitheme" + pd ) );

			FontTrueType * font = FontTrueType::New( "NotoSans-Regular", "assets/fonts/NotoSans-Regular.ttf" );

			UIThemeManager::instance()->setDefaultEffectsEnabled( true )->setDefaultTheme( theme )->setDefaultFont( font )->add( theme );
		}

		Editor = MapEditor::New();

		win->runMainLoop( &mainLoop );
	}

	Engine::destroySingleton();

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
