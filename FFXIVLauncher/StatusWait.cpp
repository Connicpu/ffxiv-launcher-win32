#include "Common.h"
#include "StatusWait.h"
#include "resource.h"
#include "Login.h"

static INT_PTR CALLBACK DialogHandler(HWND dialog, UINT msg, WPARAM wp, LPARAM lp) noexcept;
static void InitializeUI(HWND dialog) noexcept;

bool ShowStatusWaitDialog(HINSTANCE hinst) noexcept
{
    const auto result = DialogBoxW(hinst, MAKEINTRESOURCE(IDD_LOBBYSERVER), nullptr, DialogHandler);
    return false;
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
                case IDCANCEL:
                {
                    EndDialog(dialog, IDCANCEL);
                    return TRUE;
                }
            }
            return FALSE;
        }

        case WM_TIMER:
        {
            if (wp == 69)
            {
                if (IsLobbyServerReady())
                {
                    EndDialog(dialog, IDOK);
                }
                return TRUE;
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

    SetTimer(dialog, /*id=*/69, 20'000/*ms*/, nullptr);
    SendDlgItemMessageW(dialog, IDC_PROGRESS1, PBM_SETMARQUEE, TRUE, 1);
}
