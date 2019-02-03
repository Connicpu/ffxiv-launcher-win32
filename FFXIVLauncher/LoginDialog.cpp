#include "Common.h"
#include "LoginDialog.h"
#include "resource.h"
#include "Credentials.h"

static void InitializeUI(HWND dialog)
{
    SendMessageW(GetDlgItem(dialog, IDC_REMUSRBOX), BM_SETCHECK, CREDENTIALS.REMEMBER_USERNAME, 0);
    SendMessageW(GetDlgItem(dialog, IDC_REMPWDBOX), BM_SETCHECK, CREDENTIALS.REMEMBER_PASSWORD, 0);
    SendMessageW(GetDlgItem(dialog, IDC_USEOTPBOX), BM_SETCHECK, CREDENTIALS.USE_OTP, 0);

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
}

static void ReadUIValues(HWND dialog)
{
    LRESULT state;
    std::vector<wchar_t> wTemp;

    // Remember Username
    state = SendMessageW(GetDlgItem(dialog, IDC_REMUSRBOX), BM_GETCHECK, 0, 0);
    CREDENTIALS.REMEMBER_USERNAME = state != 0;

    // Remember Password
    state = SendMessageW(GetDlgItem(dialog, IDC_REMPWDBOX), BM_GETCHECK, 0, 0);
    CREDENTIALS.REMEMBER_PASSWORD = state != 0;

    // Use OTP
    state = SendMessageW(GetDlgItem(dialog, IDC_USEOTPBOX), BM_GETCHECK, 0, 0);
    CREDENTIALS.USE_OTP = state != 0;

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
}

static INT_PTR CALLBACK DialogHandler(HWND dialog, UINT msg, WPARAM wp, LPARAM lp)
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
                case IDC_CANCELBTN:
                {
                    EndDialog(dialog, IDCANCEL);
                    return TRUE;
                }

                case IDC_LAUNCHBTN:
                {
                    ReadUIValues(dialog);
                    EndDialog(dialog, IDOK);
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

bool ShowLoginDialog(HINSTANCE hinst) noexcept
{
    const auto result = DialogBoxW(hinst, MAKEINTRESOURCE(IDD_LAUNCHERDLG), nullptr, DialogHandler);
    return result == IDOK;
}
