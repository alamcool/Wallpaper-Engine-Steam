#include "WallpaperEngine.h"
#include "Config.h"
#include <thread>

WallpaperEngine& WallpaperEngine::GetInstance() {
    static WallpaperEngine instance;
    return instance;
}

void WallpaperEngine::Initialize(HINSTANCE hInstance, int displayMode) {
    m_hInstance = hInstance;
    m_displayMode = displayMode;

    RegisterWindowClass();

    EnumDisplayMonitors(NULL, NULL, [](HMONITOR hMon, HDC hdc, LPRECT lprcMonitor, LPARAM lParam) -> BOOL {
        auto* self = (WallpaperEngine*)lParam;
        MONITORINFOEX info;
        info.cbSize = sizeof(info);
        GetMonitorInfo(hMon, &info);
        self->m_monitors.push_back({
            info.szDevice,
            *lprcMonitor,
            lprcMonitor->right - lprcMonitor->left,
            lprcMonitor->bottom - lprcMonitor->top
        });
        return TRUE;
    }, (LPARAM)this);

    CreateWallpaperWindow();
    SetWindowPos(m_hwnd, HWND_BOTTOM, 0, 0,
        GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        SWP_NOACTIVATE | SWP_SHOWWINDOW);

    m_isRunning = true;
}

void WallpaperEngine::RegisterWindowClass() {
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "WallpaperEngineWindow";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassExA(&wc);
}

void WallpaperEngine::CreateWallpaperWindow() {
    m_hwnd = CreateWindowExA(
        WS_EX_TOOLWINDOW,
        "WallpaperEngineWindow", "WallpaperEngine",
        WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, m_hInstance, NULL
    );

    HWND progman = FindWindowA("Progman", NULL);
    HWND defView = FindWindowExA(progman, NULL, "SHELLDLL_DefView", NULL);
    HWND workerW = NULL;

    if (defView) {
        SendMessageTimeout(defView, 0x052C, 0, 0, SMTO_NORMAL, 1000, NULL);
        workerW = FindWindowExA(NULL, progman, "WorkerW", NULL);
        while (workerW) {
            HWND test = FindWindowExA(workerW, NULL, "SHELLDLL_DefView", NULL);
            if (!test) {
                HWND nextWorker = FindWindowExA(NULL, workerW, "WorkerW", NULL);
                if (nextWorker) workerW = nextWorker;
                break;
            }
            workerW = FindWindowExA(NULL, workerW, "WorkerW", NULL);
        }
    }

    if (workerW) {
        SetParent(m_hwnd, workerW);
    }
}

void WallpaperEngine::SetActiveWallpaper(const std::string& path) {
    m_currentWallpaperPath = path;
    m_wallpaperChanged = true;
}

void WallpaperEngine::Run() {
    MSG msg;
    while (m_isRunning && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (m_wallpaperChanged) {
            LoadWallpaper(m_currentWallpaperPath);
            m_wallpaperChanged = false;
        }

        if (ShouldPauseForFullscreen()) {
            if (m_isPlaying) {
                PausePlayback();
            }
        } else {
            if (!m_isPlaying && m_autoResume) {
                ResumePlayback();
            }
        }
    }
}

bool WallpaperEngine::ShouldPauseForFullscreen() const {
    if (!m_pauseOnFullscreen) return false;

    HWND foreground = GetForegroundWindow();
    if (!foreground) return false;

    RECT rect;
    GetClientRect(foreground, &rect);
    LONG w = rect.right - rect.left;
    LONG h = rect.bottom - rect.top;

    if (w >= GetSystemMetrics(SM_CXSCREEN) && h >= GetSystemMetrics(SM_CYSCREEN)) {
        return true;
    }
    return false;
}

void WallpaperEngine::LoadWallpaper(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) return;

    std::string ext = path.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".mp4" || ext == ".webm" || ext == ".mkv" || ext == ".avi") {
        m_wallpaperType = WallpaperType::Video;
    } else if (ext == ".html" || ext == ".htm") {
        m_wallpaperType = WallpaperType::Web;
    } else if (ext == ".jpg" || ext == ".png" || ext == ".bmp" || ext == ".gif") {
        m_wallpaperType = WallpaperType::Image;
    } else if (ext == ".pkg") {
        m_wallpaperType = WallpaperType::Scene;
    } else {
        m_wallpaperType = WallpaperType::Unknown;
    }

    m_currentWallpaperPath = path;
}

void WallpaperEngine::PausePlayback() {
    m_isPlaying = false;
}

void WallpaperEngine::ResumePlayback() {
    m_isPlaying = true;
}

void WallpaperEngine::Shutdown() {
    m_isRunning = false;
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}
