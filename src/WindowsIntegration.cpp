#include "WindowsIntegration.h"
#include "Config.h"
#include <shlobj.h>

#pragma comment(lib, "shell32.lib")

WindowsIntegration& WindowsIntegration::GetInstance() {
    static WindowsIntegration instance;
    return instance;
}

void WindowsIntegration::InstallStartupTask() {
    char appPath[MAX_PATH];
    GetModuleFileNameA(nullptr, appPath, MAX_PATH);

    ITaskService* pService = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TaskScheduler, nullptr,
        CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);

    if (FAILED(hr)) return;

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) { pService->Release(); return; }

    ITaskFolder* pFolder = nullptr;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pFolder);
    if (FAILED(hr)) { pService->Release(); return; }

    ITaskDefinition* pTask = nullptr;
    hr = pService->NewTask(0, &pTask);
    if (FAILED(hr)) { pFolder->Release(); pService->Release(); return; }

    IRegistrationInfo* pRegInfo = nullptr;
    pTask->get_RegistrationInfo(&pRegInfo);
    if (pRegInfo) {
        pRegInfo->put_Description(_bstr_t(L"Wallpaper Engine - Animated Desktop Wallpapers"));
        pRegInfo->Release();
    }

    ITriggerCollection* pTriggers = nullptr;
    pTask->get_Triggers(&pTriggers);
    ITrigger* pTrigger = nullptr;
    pTriggers->Create(TASK_TRIGGER_LOGON, &pTrigger);
    pTrigger->Release();
    pTriggers->Release();

    IActionCollection* pActions = nullptr;
    pTask->get_Actions(&pActions);
    IAction* pAction = nullptr;
    pActions->Create(TASK_ACTION_EXEC, &pAction);
    IExecAction* pExecAction = nullptr;
    pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
    pExecAction->put_Path(_bstr_t(appPath));
    pExecAction->Release();
    pAction->Release();
    pActions->Release();

    IRegisteredTask* pRegisteredTask = nullptr;
    pFolder->RegisterTaskDefinition(
        _bstr_t(L"WallpaperEngine"),
        pTask, TASK_CREATE_OR_UPDATE,
        _variant_t(), _variant_t(),
        TASK_LOGON_INTERACTIVE_TOKEN,
        _variant_t(L""),
        &pRegisteredTask
    );

    if (pRegisteredTask) pRegisteredTask->Release();
    pTask->Release();
    pFolder->Release();
    pService->Release();
}

void WindowsIntegration::RemoveStartupTask() {
    ITaskService* pService = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TaskScheduler, nullptr,
        CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
    if (FAILED(hr)) return;

    pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());

    ITaskFolder* pFolder = nullptr;
    pService->GetFolder(_bstr_t(L"\\"), &pFolder);
    if (pFolder) {
        pFolder->DeleteTask(_bstr_t(L"WallpaperEngine"), 0);
        pFolder->Release();
    }
    pService->Release();
}

void WindowsIntegration::SetWallpaper(const std::string& path) {
    SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (void*)path.c_str(),
        SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
}

void WindowsIntegration::SetMultiMonitorWallpaper(int monitorIndex, const std::string& path) {
    EnumDisplayMonitors(NULL, NULL, [](HMONITOR hMon, HDC, LPRECT, LPARAM lParam) -> BOOL {
        auto* data = (MonitorWallpaperData*)lParam;
        if (data->currentIndex == data->targetIndex) {
            MONITORINFO info;
            info.cbSize = sizeof(info);
            GetMonitorInfo(hMon, &info);

            HDC hdcScreen = GetDC(NULL);
            HDC hdcMem = CreateCompatibleDC(hdcScreen);

            HBITMAP hBmp = (HBITMAP)LoadImageA(NULL, data->path.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
            if (hBmp) {
                HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hBmp);
                StretchBlt(hdcScreen, info.rcMonitor.left, info.rcMonitor.top,
                    info.rcMonitor.right - info.rcMonitor.left,
                    info.rcMonitor.bottom - info.rcMonitor.top,
                    hdcMem, 0, 0, 1920, 1080, SRCCOPY);
                SelectObject(hdcMem, hOld);
                DeleteObject(hBmp);
            }

            DeleteDC(hdcMem);
            ReleaseDC(NULL, hdcScreen);
            return FALSE;
        }
        data->currentIndex++;
        return TRUE;
    }, (LPARAM)&MonitorWallpaperData{monitorIndex, 0, path});
}

bool WindowsIntegration::IsRunningAtStartup() {
    ITaskService* pService = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TaskScheduler, nullptr,
        CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
    if (FAILED(hr)) return false;

    pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());

    ITaskFolder* pFolder = nullptr;
    pService->GetFolder(_bstr_t(L"\\"), &pFolder);
    if (!pFolder) { pService->Release(); return false; }

    IRegisteredTask* pTask = nullptr;
    hr = pFolder->GetTask(_bstr_t(L"WallpaperEngine"), &pTask);
    pFolder->Release();
    pService->Release();

    bool exists = SUCCEEDED(hr);
    if (pTask) pTask->Release();
    return exists;
}
