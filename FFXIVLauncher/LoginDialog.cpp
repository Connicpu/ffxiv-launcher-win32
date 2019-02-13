#include "Common.h"
#include "LoginDialog.h"
#include "resource.h"
#include "Credentials.h"
#include "GameDirSearch.h"
#include "Login.h"

static void InitCheckboxes(HWND dialog) noexcept;
static void SetSkipBox(HWND dialog) noexcept;
static void SetUserHide(HWND dialog) noexcept;
static void SetGDir(HWND dialog) noexcept;
static void InitializeUI(HWND dialog) noexcept;
static void ReadUIValues(HWND dialog) noexcept;
static void ReadCheckboxes(HWND dialog) noexcept;
static void SelectFolder(HWND dialog) noexcept;
static bool IsWine() noexcept;
static INT_PTR CALLBACK DialogHandler(HWND dialog, UINT msg, WPARAM wp, LPARAM lp) noexcept;

bool ShowLoginDialog(HINSTANCE hinst) noexcept
{
    const auto result = DialogBoxW(hinst, MAKEINTRESOURCE(IDD_LAUNCHERDLG), nullptr, DialogHandler);
    return result == IDOK;
}

static INT_PTR CALLBACK DialogHandler(HWND dialog, UINT msg, WPARAM wp, LPARAM lp) noexcept
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        InitializeUI(dialog);
        return TRUE;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wp))
        {
        case 2:
        case IDC_CANCELBTN:
        {
            EndDialog(dialog, IDCANCEL);
            return TRUE;
        }

        case 1:
        case IDC_LAUNCHBTN:
        {
            ReadUIValues(dialog);
            if (CREDENTIALS.username.empty() || CREDENTIALS.password.empty())
            {
                MessageBoxW(dialog, L"Please enter a username and password", L"Invalid Entry", MB_ICONWARNING);
            }
            else if (!fs::exists(CREDENTIALS.game_dir / "game") || !fs::exists(CREDENTIALS.game_dir / "boot"))
            {
                MessageBoxW(
                    dialog,
                    L"Please select the directory where the game files are located. "
                    L"There should be 2 folders called 'game' and 'boot' in the selected directory.",
                    L"Invalid Directory",
                    MB_ICONERROR
                );
            }
            else
            {
                EndDialog(dialog, IDOK);
            }
            return TRUE;
        }

        case IDC_SETGAMEDIRBTN:
        {
            SelectFolder(dialog);
            SetGDir(dialog);
            return TRUE;
        }

        case IDC_INSTALLBTN:
        {
            auto res = MessageBoxW(
                dialog,
                L"Are you sure you want to overwrite ffxivboot.exe?",
                L"Are you sure",
                MB_YESNO
            );
            if (res != IDYES) return TRUE;
            try
            {
                auto bootdir = CREDENTIALS.game_dir / "boot";
                auto boot = bootdir / "ffxivboot.exe";
                auto bootold = bootdir / "ffxivboot.exe.old";
                auto self = GetSelfPath();

                if (!BootWasReplaced())
                {
                    fs::copy_file(boot, bootold, fs::copy_options::overwrite_existing);
                }
                fs::copy_file(self, boot, fs::copy_options::overwrite_existing);

                auto hInstall = GetDlgItem(dialog, IDC_INSTALLBTN);
                ShowWindow(hInstall, SW_HIDE);
            }
            catch (const fs::filesystem_error &)
            {
                MessageBoxW(
                    dialog,
                    L"Failed to install over ffxivboot.exe. "
                    L"You may need to replace it yourself or run as admin. "
                    L"Make sure to copy ffxivboot.exe as ffxivboot.exe.old or "
                    L"you may not be able to update without getting a new copy "
                    L"of the game installed, if you decide to do it manually.",
                    L"Install failed",
                    MB_ICONERROR
                );
            }

            return TRUE;
        }

        // Update the eligibility of the Skip box when these are changed.
        case IDC_REMUSRBOX:
        case IDC_REMPWDBOX:
        {
            ReadCheckboxes(dialog);
            SetSkipBox(dialog);
            return TRUE;
        }

        case IDC_HIDEUSRBOX:
        {
            ReadCheckboxes(dialog);
            SetUserHide(dialog);
            return TRUE;
        }
        }
        return FALSE;
    }

    case WM_CLOSE:
    {
        EndDialog(dialog, IDCANCEL);
        return TRUE;
    }

    default:
    {
        return FALSE;
    }
    }
}

static void InitializeUI(HWND dialog) noexcept
{
    auto self_path = GetSelfPath();
    auto boot = fs::canonical(CREDENTIALS.game_dir / "boot/ffxivboot.exe");

    if (self_path != boot)
    {
        auto selfhash = HashFile(self_path);
        auto boothash = HashFile(boot);
        if (selfhash != boothash)
        {
            auto hInstall = GetDlgItem(dialog, IDC_INSTALLBTN);
            auto style = GetWindowLongW(hInstall, GWL_STYLE);
            style |= WS_VISIBLE;
            SetWindowLongW(hInstall, GWL_STYLE, style);
        }
    }

    auto appIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCE(IDI_APPICON));
    SendMessageW(dialog, WM_SETICON, ICON_BIG, (LPARAM)appIcon);

    InitCheckboxes(dialog);

    SetGDir(dialog);

    if (CREDENTIALS.remember_username)
    {
        auto wtext = UTF_CONVERT.from_bytes(CREDENTIALS.username);
        SetWindowTextW(GetDlgItem(dialog, IDC_USERNAMEBOX), wtext.c_str());
    }
    if (CREDENTIALS.remember_password)
    {
        auto wtext = UTF_CONVERT.from_bytes(CREDENTIALS.password);
        SetWindowTextW(GetDlgItem(dialog, IDC_PASSWORDBOX), wtext.c_str());
    }

    const auto dpi = GetDpiForWindow(dialog);
    const auto iconSize = (16 * dpi) / 96;

    auto hHide = GetDlgItem(dialog, IDC_HIDEUSRBOX);
    auto hideIcon = LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_HIDEICON), IMAGE_ICON, iconSize, iconSize, LR_DEFAULTCOLOR);
    SendMessageW(hHide, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hideIcon);
}

static void InitCheckboxes(HWND dialog) noexcept
{
    SendDlgItemMessageW(dialog, IDC_HIDEUSRBOX, BM_SETCHECK, CREDENTIALS.hide_username, 0);
    SendDlgItemMessageW(dialog, IDC_REMUSRBOX, BM_SETCHECK, CREDENTIALS.remember_username, 0);
    SendDlgItemMessageW(dialog, IDC_REMPWDBOX, BM_SETCHECK, CREDENTIALS.remember_password, 0);
    SendDlgItemMessageW(dialog, IDC_USEOTPBOX, BM_SETCHECK, CREDENTIALS.use_otp, 0);

    SetUserHide(dialog);
    SetSkipBox(dialog);
}

static void SetSkipBox(HWND dialog) noexcept
{
    const auto hSkip = GetDlgItem(dialog, IDC_SKIPLOGINBOX);

    if (CREDENTIALS.remember_username &&CREDENTIALS.remember_password)
    {
        EnableWindow(hSkip, TRUE);
    }
    else
    {
        EnableWindow(hSkip, FALSE);
        CREDENTIALS.skip_login = false;
    }

    SendMessageW(hSkip, BM_SETCHECK, CREDENTIALS.skip_login, 0);
}

static void SetUserHide(HWND dialog) noexcept
{
    const auto hUser = GetDlgItem(dialog, IDC_USERNAMEBOX);
    SendMessageW(hUser, EM_SETPASSWORDCHAR, CREDENTIALS.hide_username ? 9679 : 0, 0);
    EnableWindow(hUser, FALSE);
    EnableWindow(hUser, TRUE);
}

void SetGDir(HWND dialog) noexcept
{
    const wchar_t *gdir = CREDENTIALS.game_dir.c_str();
    if (!IsGameDir(CREDENTIALS.game_dir))
    {
        gdir = L"Please select a directory";
    }
    SetWindowTextW(GetDlgItem(dialog, IDC_GAMEDIR), gdir);
}

static void ReadUIValues(HWND dialog) noexcept
{
    std::vector<wchar_t> wTemp;

    // Username
    const auto hUsername = GetDlgItem(dialog, IDC_USERNAMEBOX);
    const auto unameLen = GetWindowTextLengthW(hUsername) + 1;
    wTemp.resize(unameLen);
    GetWindowTextW(hUsername, &wTemp.at(0), unameLen);
    CREDENTIALS.username = UTF_CONVERT.to_bytes(wTemp.data());

    // Password
    const auto hPassword = GetDlgItem(dialog, IDC_PASSWORDBOX);
    const auto passLen = GetWindowTextLengthW(hPassword) + 1;
    wTemp.resize(passLen);
    GetWindowTextW(hPassword, &wTemp.at(0), passLen);
    CREDENTIALS.password = UTF_CONVERT.to_bytes(wTemp.data());

    // Checkboxes
    ReadCheckboxes(dialog);
}

static void ReadCheckboxes(HWND dialog) noexcept
{
    LRESULT state;

    // Remember Username
    state = SendDlgItemMessageW(dialog, IDC_REMUSRBOX, BM_GETCHECK, 0, 0);
    CREDENTIALS.remember_username = state != 0;

    // Remember Password
    state = SendDlgItemMessageW(dialog, IDC_REMPWDBOX, BM_GETCHECK, 0, 0);
    CREDENTIALS.remember_password = state != 0;

    // Use OTP
    state = SendDlgItemMessageW(dialog, IDC_USEOTPBOX, BM_GETCHECK, 0, 0);
    CREDENTIALS.use_otp = state != 0;

    // Skip
    state = SendDlgItemMessageW(dialog, IDC_SKIPLOGINBOX, BM_GETCHECK, 0, 0);
    CREDENTIALS.skip_login = state != 0;

    // Hide Username
    state = SendDlgItemMessageW(dialog, IDC_HIDEUSRBOX, BM_GETCHECK, 0, 0);
    CREDENTIALS.hide_username = state != 0;
}

static void SelectFolder(HWND dialog) noexcept
{
    if (IsWine())
    {
        wchar_t path[MAX_PATH] = { 0 };

        BROWSEINFOW info = { dialog };
        info.pszDisplayName = path;
        info.lpszTitle = L"FFXIV Game Directory";
        info.ulFlags = BIF_RETURNONLYFSDIRS;

        if (SHBrowseForFolderW(&info))
        {
            CREDENTIALS.game_dir = path;
        }
    }
    else
    {
        HRESULT hr;

        ATL::CComPtr<IFileOpenDialog> fd;
        hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fd));
        if (FAILED(hr)) return;

        hr = fd->SetOptions(FOS_PICKFOLDERS);
        if (FAILED(hr)) return;

        hr = fd->Show(dialog);
        if (FAILED(hr)) return; // Failure here means the user hit "Cancel"

        ATL::CComPtr<IShellItem> result;
        hr = fd->GetResult(&result);
        if (FAILED(hr)) return;
         
        wchar_t *path;
        hr = result->GetDisplayName(SIGDN_FILESYSPATH, &path);
        if (FAILED(hr)) return;

        CREDENTIALS.game_dir = path;
        CoTaskMemFree(path);
    }
}

bool IsWine() noexcept
{
    return !!GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_version");
}
