#include "Common.h"

#include "LoginDialog.h"
#include "OTPDialog.h"
#include "Credentials.h"
#include "Login.h"
#include "GameDirSearch.h"
#include "StatusWait.h"

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
            case LoginResult::Maintenance:
                if (!ShowStatusWaitDialog(hinst))
                {
                    break;
                }
                // Intentional fallthrough: the server is back up now

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

static int UpdateError(int err)
{
    auto msg = L"An error occurred while trying to launch the updater: " + std::to_wstring(err);
    MessageBoxW(nullptr, msg.c_str(), L"Update error", MB_ICONERROR);
    return -1;
}

static HANDLE GetProcess(const fs::path &path)
{
    auto name = path.filename();

    auto hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (!hSnap)
    {
        UpdateError(-1);
        _exit(1);
    }

    PROCESSENTRY32W entry = { sizeof(entry) };
    if (!Process32FirstW(hSnap, &entry)) return nullptr;
    while (Process32NextW(hSnap, &entry))
    {
        if (StrCmpW(entry.szExeFile, name.c_str()) == 0)
        {
            wchar_t buf[2048];
            DWORD buflen = 2048;
            auto hProc = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
            if (!QueryFullProcessImageNameW(hProc, 0, buf, &buflen))
            {
                UpdateError(-2);
                _exit(1);
            }
            buf[buflen] = 0;
            fs::path procPath = fs::canonical(buf);
            if (path == procPath)
            {
                return hProc;
            }
            CloseHandle(hProc);
        }
    }
    return nullptr;
}

static int RunUpdateWatcher(std::string_view cmd)
{
    using namespace std::chrono_literals;

    auto bootdir = CREDENTIALS.game_dir / "boot";
    auto boot = bootdir / "ffxivboot.exe";
    auto bootold = bootdir / "ffxivboot.exe.old";
    auto launch = fs::canonical(bootdir / "ffxivlauncher.exe");

    if (GetProcess(launch))
    {
        MessageBoxW(nullptr, L"The FFXIV Launcher is already running", L"Already running", MB_ICONERROR);
    }

    auto pid = atoi(cmd.substr(cmd.find_first_of(' ') + 1).data());
    auto hWait = OpenSemaphoreW(SEMAPHORE_MODIFY_STATE, FALSE, L"Global\\FFXIV_LAUNCHER_TEMP_UPDATER_WAIT");
    if (!hWait) return UpdateError(1);
    auto hProc = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!hProc) return UpdateError(2);
    if (!ReleaseSemaphore(hWait, 1, nullptr)) return UpdateError(3);
    if (WaitForSingleObject(hProc, INFINITE) != 0) return UpdateError(4);
    CloseHandle(hWait);

    try
    {
        fs::copy_file(bootold, boot, fs::copy_options::overwrite_existing);
    }
    catch (const fs::filesystem_error &)
    {
        return UpdateError(12);
    }

    STARTUPINFOW startup = { sizeof(startup) };
    PROCESS_INFORMATION info = { 0 };
    if (!CreateProcessW(boot.c_str(), nullptr, nullptr, nullptr, 0, 0, nullptr, bootdir.c_str(), &startup, &info)) return UpdateError(6);
    if (!info.hProcess) return UpdateError(20);
    if (WaitForSingleObject(info.hProcess, INFINITE) != 0) return UpdateError(21);

    HANDLE hLauncher = nullptr;
    auto start_time = std::chrono::system_clock::now();
    for (;;)
    {
        if (hLauncher = GetProcess(launch)) break;

        auto time = std::chrono::system_clock::now() - start_time;
        if (time > 30'000ms)
        {
            return UpdateError(31);
        }
        Sleep(200);
    }

    if (WaitForSingleObject(hLauncher, INFINITE) != 0) return UpdateError(32);

    auto self_path = GetSelfPath();

    try
    {
        fs::copy_file(boot, bootold, fs::copy_options::overwrite_existing);
        fs::copy_file(self_path, boot, fs::copy_options::overwrite_existing);
    }
    catch (const fs::filesystem_error &)
    {
        return UpdateError(40);
    }

    return 0;
}
