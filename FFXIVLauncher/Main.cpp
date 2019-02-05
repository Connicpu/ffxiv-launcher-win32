#include "Common.h"

#include "LoginDialog.h"
#include "OTPDialog.h"
#include "Credentials.h"
#include "Login.h"
#include "GameDirSearch.h"

static int RunLauncher(HINSTANCE hinst);
static int RunUpdateWatcher(std::string_view cmd);

INT WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR pcmd, int)
{
    CoInitialize(nullptr);
    CREDENTIALS.Load();

    std::string_view cmd(pcmd);
    if (cmd.find("update_ffxivboot", 0) == 0)
    {
        return RunUpdateWatcher(cmd);
    }
    else if (cmd != "relaunch")
    {
        // TEST
        LaunchUpdater();
        return 0;
    }
    else
    {
        return RunLauncher(hinst);
    }
}

static int RunLauncher(HINSTANCE hinst)
{
    if (!IsGameDir(CREDENTIALS.game_dir))
    {
        if (auto game_dir = TryFindGameDir())
        {
            CREDENTIALS.game_dir = *game_dir;
        }
    }

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
            {
                auto res = MessageBoxW(nullptr, L"An update is required. Would you like to launch the updater?", L"Update required", MB_YESNO | MB_ICONINFORMATION);
                if (res == IDYES)
                {
                    LaunchUpdater();
                }
                break;
            }

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

int RunUpdateWatcher(std::string_view cmd)
{


    return 0;
}
