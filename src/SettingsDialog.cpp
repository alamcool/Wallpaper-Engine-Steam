#include "SettingsDialog.h"
#include "Config.h"
#include <CommCtrl.h>

SettingsDialog& SettingsDialog::GetInstance() {
    static SettingsDialog instance;
    return instance;
}

void SettingsDialog::Show(HWND parent) {
    DialogBoxParamA(
        GetModuleHandleA(nullptr),
        MAKEINTRESOURCEA(100),
        parent,
        DialogProc,
        (LPARAM)this
    );
}

INT_PTR CALLBACK SettingsDialog::DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    Config& config = Config::GetInstance();

    switch (msg) {
        case WM_INITDIALOG: {
            CheckDlgButton(hDlg, IDC_STARTUP, config.m_runOnStartup ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_PAUSE_FULLSCREEN, config.m_pauseOnFullscreen ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_RGB_CHROMA, config.m_rgbChroma ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_RGB_ICUE, config.m_rgbICue ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_RGB_HUE, config.m_rgbHue ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_HDR, config.m_hdrEnabled ? BST_CHECKED : BST_UNCHECKED);

            HWND hSlider = GetDlgItem(hDlg, IDC_QUALITY_SLIDER);
            SendMessageA(hSlider, TBM_SETRANGE, TRUE, MAKELONG(0, 4));
            SendMessageA(hSlider, TBM_SETPOS, TRUE, config.m_qualityLevel);

            return TRUE;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDOK: {
                    config.m_runOnStartup = IsDlgButtonChecked(hDlg, IDC_STARTUP) == BST_CHECKED;
                    config.m_pauseOnFullscreen = IsDlgButtonChecked(hDlg, IDC_PAUSE_FULLSCREEN) == BST_CHECKED;
                    config.m_rgbChroma = IsDlgButtonChecked(hDlg, IDC_RGB_CHROMA) == BST_CHECKED;
                    config.m_rgbICue = IsDlgButtonChecked(hDlg, IDC_RGB_ICUE) == BST_CHECKED;
                    config.m_rgbHue = IsDlgButtonChecked(hDlg, IDC_RGB_HUE) == BST_CHECKED;
                    config.m_hdrEnabled = IsDlgButtonChecked(hDlg, IDC_HDR) == BST_CHECKED;
                    config.m_qualityLevel = (int)SendMessageA(GetDlgItem(hDlg, IDC_QUALITY_SLIDER), TBM_GETPOS, 0, 0);

                    config.Save("config/settings.json");
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }
                case IDCANCEL: {
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                }
                case IDC_BROWSE_WORKSHOP: {
                    char path[MAX_PATH] = {0};
                    BROWSEINFOA bi = {};
                    bi.hwndOwner = hDlg;
                    bi.lpszTitle = "Select Steam Workshop folder";
                    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
                    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
                    if (pidl) {
                        SHGetPathFromIDListA(pidl, path);
                        SetDlgItemTextA(hDlg, IDC_WORKSHOP_PATH, path);
                        CoTaskMemFree(pidl);
                    }
                    return TRUE;
                }
            }
            break;
        }

        case WM_HSCROLL: {
            if ((HWND)lParam == GetDlgItem(hDlg, IDC_QUALITY_SLIDER)) {
                int pos = (int)SendMessageA((HWND)lParam, TBM_GETPOS, 0, 0);
                const char* labels[] = {"Low", "Medium", "High", "Very High", "Ultra"};
                SetDlgItemTextA(hDlg, IDC_QUALITY_LABEL, labels[pos]);
            }
            return TRUE;
        }
    }

    return FALSE;
}
