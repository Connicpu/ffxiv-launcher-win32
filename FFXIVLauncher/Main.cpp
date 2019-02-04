#include "Common.h"

#include "LoginDialog.h"
#include "OTPDialog.h"
#include "Credentials.h"
#include "Login.h"

INT WINAPI WinMain(_In_ HINSTANCE hinst, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
    CoInitialize(nullptr);

    CREDENTIALS.Load();

    for (int i = 0;; i++)
    {
        if (CREDENTIALS.username.empty() || CREDENTIALS.password.empty() || !CREDENTIALS.skip_login || i)
        {
            if (!ShowLoginDialog(hinst))
            {
                return 0;
            }
        }

        CREDENTIALS.Save();

        if (CREDENTIALS.use_otp)
        {
            switch (ShowOTPDialog(hinst))
            {
                case 0: return 0;
                case 1: break;
                case 2: continue;

                default: abort(); // unreachable
            }
        }

        switch (PerformLogin())
        {
            case LoginResult::Success:
                LaunchGame();
                break;

            case LoginResult::UpdateRequired:
                LaunchUpdater();
                break;

            case LoginResult::InvalidCredentials:
                MessageBoxW(nullptr, L"Your username, password, or one-time password was incorrect.", L"Login failed", MB_ICONERROR);
                continue;

            case LoginResult::TooManyLogins:
                MessageBoxW(nullptr, L"Login failed because of too many failed login attempts.", L"Login failed", MB_ICONERROR);
                continue;

            case LoginResult::NetworkError:
                MessageBoxW(nullptr, L"A network error occurred while trying to log in.", L"Login failed", MB_ICONERROR);
                continue;

            case LoginResult::CuckedBySquare:
                MessageBoxW(
                    nullptr,
                    L"It seems Square Enix has changed something about the login procedure. "
                    L"Update this app or use the normal launcher.",
                    L"Login failed",
                    MB_ICONERROR
                );
                LaunchUpdater();
                break;

            default: abort(); // unreachable
        }

        break;
    }

    return 0;
}
