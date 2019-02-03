#include <Windows.h>
#include "resource.h"
#include <string>
#include <codecvt>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

static bool REMEMBER_USERNAME = true;
static bool REMEMBER_PASSWORD = false;
static bool USE_OTP = true;

static std::string USERNAME = "Someone1234";
static std::string PASSWORD;

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> UTF_CONVERTER;

static void InitializeUI(HWND dialog)
{
    SendMessageW(GetDlgItem(dialog, IDC_REMUSRBOX), BM_SETCHECK, REMEMBER_USERNAME, 0);
    SendMessageW(GetDlgItem(dialog, IDC_REMPWDBOX), BM_SETCHECK, REMEMBER_PASSWORD, 0);
    SendMessageW(GetDlgItem(dialog, IDC_USEOTPBOX), BM_SETCHECK, USE_OTP, 0);

    if (REMEMBER_USERNAME)
    {
        SetWindowTextA(GetDlgItem(dialog, IDC_USERNAMEBOX), USERNAME.c_str());
    }
    if (REMEMBER_PASSWORD)
    {
        SetWindowTextA(GetDlgItem(dialog, IDC_PASSWORDBOX), PASSWORD.c_str());
    }
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
                case IDC_REMUSRBOX:
                {
                    auto state = SendMessageW(GetDlgItem(dialog, IDC_REMUSRBOX), BM_GETCHECK, 0, 0);
                    REMEMBER_USERNAME = state != 0;
                    return TRUE;
                }

                case IDC_REMPWDBOX:
                {
                    auto state = SendMessageW(GetDlgItem(dialog, IDC_REMPWDBOX), BM_GETCHECK, 0, 0);
                    REMEMBER_PASSWORD = state != 0;
                    return TRUE;
                }

                case IDC_USEOTPBOX:
                {
                    auto state = SendMessageW(GetDlgItem(dialog, IDC_USEOTPBOX), BM_GETCHECK, 0, 0);
                    USE_OTP = state != 0;
                    return TRUE;
                }

                case IDC_CANCELBTN:
                {
                    EndDialog(dialog, IDCANCEL);
                    return TRUE;
                }

                case IDC_LAUNCHBTN:
                {
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

INT WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE,
    _In_ LPSTR,
    _In_ int)
{
    auto result = DialogBoxW(hInstance, MAKEINTRESOURCE(IDD_LAUNCHERDLG), nullptr, DialogHandler);

    return 0;
}
