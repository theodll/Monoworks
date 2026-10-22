#include <mwpch.hh>

#include <debug/MessageBox.hh>

namespace Monoworks
{
	EMessageBoxButton MWAPI_CALL ShowMessageBox(
		std::string_view text,
		std::string_view caption,
		EMessageBoxFlags useFlags,
		EMessageBoxCreationFlags creationFlags ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;

		HWND parent = nullptr;

		if ( creationFlags & MW_MESSAGE_BOX_CREATION_FLAG_SUBWINDOW )
			parent = GetForegroundWindow();

		int res = MessageBoxA( parent, text.data(), caption.data(), static_cast< UINT >( useFlags ) );

		switch ( res )
		{
		case IDABORT:
			return MW_MESSAGE_BOX_BUTTON_ABORT;
		case IDCANCEL: 
			return MW_MESSAGE_BOX_BUTTON_CANCEL;
		case IDCONTINUE:
			return MW_MESSAGE_BOX_BUTTON_CONTINUE;
		case IDIGNORE:
			return MW_MESSAGE_BOX_BUTTON_IGNORE;
		case IDNO:
			return MW_MESSAGE_BOX_BUTTON_NO;
		case IDOK:
			return MW_MESSAGE_BOX_BUTTON_OK;
		case IDRETRY:
			return MW_MESSAGE_BOX_BUTTON_RETRY;
		case IDTRYAGAIN:
			return MW_MESSAGE_BOX_BUTTON_TRY_AGAIN;
		case IDYES:
			return MW_MESSAGE_BOX_BUTTON_YES;
		case 0:
			return MW_MESSAGE_BOX_BUTTON_FAILED;
		default:
			MW_ERROR( "MessageBoxA returned invalid return value.");
			return MW_MESSAGE_BOX_BUTTON_FAILED;
		}
	};

}
