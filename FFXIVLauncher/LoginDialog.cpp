#include "Common.h"
#include "LoginDialog.h"
#include "resource.h"
#include "Credentials.h"

static void InitCheckboxes(HWND dialog) noexcept;
static void SetSkipBox(HWND dialog) noexcept;
static void SetUserHide(HWND dialog) noexcept;
static void SetGDir(HWND dialog) noexcept;
static void InitializeUI(HWND dialog) noexcept;
static void ReadUIValues(HWND dialog) noexcept;
static void ReadCheckboxes(HWND dialog) noexcept;
static void SelectFolder(HWND dialog) noexcept;
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
                    if (CREDENTIALS.USERNAME.empty() || CREDENTIALS.PASSWORD.empty())
                    {
                        MessageBoxW(dialog, L"Please enter a username and password", L"Invalid Entry", MB_ICONWARNING);
                    }
                    else if (!fs::exists(CREDENTIALS.GAME_DIR / "game") || !fs::exists(CREDENTIALS.GAME_DIR / "boot"))
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
    auto appIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCE(IDI_APPICON));
    SendMessageW(dialog, WM_SETICON, ICON_BIG, (LPARAM)appIcon);

    InitCheckboxes(dialog);

    SetGDir(dialog);

    if (CREDENTIALS.REMEMBER_USERNAME)
    {
        auto wtext = UTF_CONVERT.from_bytes(CREDENTIALS.USERNAME);
        SetWindowTextW(GetDlgItem(dialog, IDC_USERNAMEBOX), wtext.c_str());
    }
    if (CREDENTIALS.REMEMBER_PASSWORD)
    {
        auto wtext = UTF_CONVERT.from_bytes(CREDENTIALS.PASSWORD);
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
    SendDlgItemMessageW(dialog, IDC_HIDEUSRBOX, BM_SETCHECK, CREDENTIALS.HIDE_USERNAME, 0);
    SendDlgItemMessageW(dialog, IDC_REMUSRBOX, BM_SETCHECK, CREDENTIALS.REMEMBER_USERNAME, 0);
    SendDlgItemMessageW(dialog, IDC_REMPWDBOX, BM_SETCHECK, CREDENTIALS.REMEMBER_PASSWORD, 0);
    SendDlgItemMessageW(dialog, IDC_USEOTPBOX, BM_SETCHECK, CREDENTIALS.USE_OTP, 0);

    SetUserHide(dialog);
    SetSkipBox(dialog);
}

static void SetSkipBox(HWND dialog) noexcept
{
    const auto hSkip = GetDlgItem(dialog, IDC_SKIPLOGINBOX);

    if (CREDENTIALS.REMEMBER_USERNAME &&CREDENTIALS.REMEMBER_PASSWORD)
    {
        EnableWindow(hSkip, TRUE);
    }
    else
    {
        EnableWindow(hSkip, FALSE);
        CREDENTIALS.SKIP_LOGIN = false;
    }

    SendMessageW(hSkip, BM_SETCHECK, CREDENTIALS.SKIP_LOGIN, 0);
}

static void SetUserHide(HWND dialog) noexcept
{
    const auto hUser = GetDlgItem(dialog, IDC_USERNAMEBOX);
    SendMessageW(hUser, EM_SETPASSWORDCHAR, CREDENTIALS.HIDE_USERNAME ? 9679 : 0, 0);
    EnableWindow(hUser, FALSE);
    EnableWindow(hUser, TRUE);
}

void SetGDir(HWND dialog) noexcept
{
    auto gdir = CREDENTIALS.GAME_DIR.generic_wstring();
    if (!fs::exists(CREDENTIALS.GAME_DIR))
    {
        gdir = L"Please select a directory";
    }
    SetWindowTextW(GetDlgItem(dialog, IDC_GAMEDIR), gdir.c_str());
}

static void ReadUIValues(HWND dialog) noexcept
{
    std::vector<wchar_t> wTemp;

    // Username
    const auto hUsername = GetDlgItem(dialog, IDC_USERNAMEBOX);
    const auto unameLen = GetWindowTextLengthW(hUsername) + 1;
    wTemp.resize(unameLen);
    GetWindowTextW(hUsername, &wTemp.at(0), unameLen);
    CREDENTIALS.USERNAME = UTF_CONVERT.to_bytes(wTemp.data());

    // Password
    const auto hPassword = GetDlgItem(dialog, IDC_PASSWORDBOX);
    const auto passLen = GetWindowTextLengthW(hPassword) + 1;
    wTemp.resize(passLen);
    GetWindowTextW(hPassword, &wTemp.at(0), passLen);
    CREDENTIALS.PASSWORD = UTF_CONVERT.to_bytes(wTemp.data());

    // Checkboxes
    ReadCheckboxes(dialog);
}

static void ReadCheckboxes(HWND dialog) noexcept
{
    LRESULT state;

    // Remember Username
    state = SendDlgItemMessageW(dialog, IDC_REMUSRBOX, BM_GETCHECK, 0, 0);
    CREDENTIALS.REMEMBER_USERNAME = state != 0;

    // Remember Password
    state = SendDlgItemMessageW(dialog, IDC_REMPWDBOX, BM_GETCHECK, 0, 0);
    CREDENTIALS.REMEMBER_PASSWORD = state != 0;

    // Use OTP
    state = SendDlgItemMessageW(dialog, IDC_USEOTPBOX, BM_GETCHECK, 0, 0);
    CREDENTIALS.USE_OTP = state != 0;

    // Skip
    state = SendDlgItemMessageW(dialog, IDC_SKIPLOGINBOX, BM_GETCHECK, 0, 0);
    CREDENTIALS.SKIP_LOGIN = state != 0;

    // Hide Username
    state = SendDlgItemMessageW(dialog, IDC_HIDEUSRBOX, BM_GETCHECK, 0, 0);
    CREDENTIALS.HIDE_USERNAME = state != 0;
}

void SelectFolder(HWND dialog) noexcept
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

    CREDENTIALS.GAME_DIR = path;
    CoTaskMemFree(path);
}
