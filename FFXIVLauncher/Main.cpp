#include "Common.h"

#include "LoginDialog.h"
#include "OTPDialog.h"
#include "Credentials.h"

INT WINAPI WinMain(_In_ HINSTANCE hinst, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
    Credentials::Load();

    for (int i = 0;; i++)
    {
        if (Credentials::USERNAME.empty() || Credentials::PASSWORD.empty() || !Credentials::SKIP_LOGIN || i)
        {
            if (!ShowLoginDialog(hinst))
            {
                return 0;
            }
        }

        Credentials::Save();

        if (Credentials::USE_OTP)
        {
            switch (ShowOTPDialog(hinst))
            {
                case 0: return 0;
                case 1: break;
                case 2: continue;

                default: return -1; // unreachable?
            }
        }

        break;
    }

    // TODO: Log in and launch the game

    return 0;
}
