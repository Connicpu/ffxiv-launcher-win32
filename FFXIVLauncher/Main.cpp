#include "Common.h"

#include "LoginDialog.h"
#include "OTPDialog.h"
#include "Credentials.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

INT WINAPI WinMain(
    _In_ HINSTANCE hinst,
    _In_opt_ HINSTANCE,
    _In_ LPSTR,
    _In_ int)
{
    CREDENTIALS.Load();

    if (!ShowLoginDialog(hinst))
    {
        return 0;
    }

    CREDENTIALS.Save();

    if (CREDENTIALS.USE_OTP)
    {
        if (!ShowOTPDialog(hinst))
        {
            return 0;
        }
    }

    return 0;
}
