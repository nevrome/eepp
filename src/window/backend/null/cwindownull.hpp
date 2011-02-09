#ifndef EE_WINDOWCWINDOWNULL_HPP
#define EE_WINDOWCWINDOWNULL_HPP

#include "../../cwindow.hpp"

namespace EE { namespace Window { namespace Backend { namespace Null {

class EE_API cWindowNull : public cWindow {
	public:
		cWindowNull( WindowSettings Settings, ContextSettings Context );
		
		virtual ~cWindowNull();
		
		bool Create( WindowSettings Settings, ContextSettings Context );
		
		void ToggleFullscreen();
		
		void Caption( const std::string& Caption );
		
		std::string Caption();

		bool Icon( const std::string& Path );

		void Minimize();

		void Maximize();

		void Hide();

		void Raise();

		void Show();

		void Position( Int16 Left, Int16 Top );

		bool Active();

		bool Visible();

		eeVector2i Position();

		void Size( const Uint32& Width, const Uint32& Height );

		void Size( const Uint16& Width, const Uint16& Height, const bool& Windowed );
		
		void ShowCursor( const bool& showcursor );

		std::vector< std::pair<unsigned int, unsigned int> > GetPossibleResolutions() const;

		void SetGamma( eeFloat Red, eeFloat Green, eeFloat Blue );

		void SetCurrentContext( eeWindowContex Context );

		eeWindowContex GetContext() const;

		eeWindowHandler	GetWindowHandler();

		void SetDefaultContext();
	protected:
		friend class cClipboardNull;

		void SwapBuffers();

		void GetMainContext();
};

}}}}

#endif
