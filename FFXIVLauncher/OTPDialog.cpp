#include "Common.h"
#include "OTPDialog.h"
#include "resource.h"
#include "Credentials.h"
#include <sstream>

static INT_PTR CALLBACK DialogHandler(HWND dialog, UINT msg, WPARAM wp, LPARAM lp) noexcept;
static void InitializeUI(HWND dialog) noexcept;
static void ReadUIValues(HWND dialog) noexcept;

int ShowOTPDialog(HINSTANCE hinst) noexcept
{
    return (int)DialogBoxW(hinst, MAKEINTRESOURCE(IDD_OTPENTRY), nullptr, DialogHandler);
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
                case IDC_CANCELBTN:
                {
                    EndDialog(dialog, 0);
                    return TRUE;
                }

                case 2:
                case IDC_GOBACK:
                {
                    EndDialog(dialog, 2);
                    return TRUE;
                }

                case 1:
                case IDC_LAUNCHBTN:
                {
                    ReadUIValues(dialog);
                    if (CREDENTIALS.otp.empty())
                    {
                        MessageBoxW(dialog, L"Please enter your one-time password from the app", L"Invalid Entry", MB_OK | MB_ICONWARNING);
                    }
                    else
                    {
                        EndDialog(dialog, 1);
                    }
                    return TRUE;
                }
            }
            return FALSE;
        }

        case WM_CLOSE:
        {
            EndDialog(dialog, 0);
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
}

static void ReadUIValues(HWND dialog) noexcept
{
    std::vector<wchar_t> wTemp;

    // OTP
    const auto hOtp = GetDlgItem(dialog, IDC_OTPBOX);
    const auto otpLen = GetWindowTextLengthW(hOtp) + 1;
    wTemp.resize(otpLen);
    GetWindowTextW(hOtp, &wTemp.at(0), otpLen);
    CREDENTIALS.otp = UTF_CONVERT.to_bytes(wTemp.data());
}
