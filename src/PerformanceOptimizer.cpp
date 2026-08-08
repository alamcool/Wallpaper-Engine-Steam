#include "PerformanceOptimizer.h"
#include "Config.h"

PerformanceOptimizer& PerformanceOptimizer::GetInstance() {
    static PerformanceOptimizer instance;
    return instance;
}

void PerformanceOptimizer::Initialize() {
    m_gpuUsageLimit = 30;
    m_cpuUsageLimit = 10;
    m_ramLimitMB = 512;
    m_cacheSizeMB = 2048;
    m_pauseOnFullscreen = true;
    m_pauseOnBattery = false;
    m_qualityLevel = 2;
    m_frameSkipEnabled = true;
    m_lastGpuUsage = 0.0f;
    m_lastCpuUsage = 0.0f;
    m_lastRamUsageMB = 0;
}

bool PerformanceOptimizer::ShouldPause() {
    if (m_pauseOnFullscreen && IsFullscreenAppActive()) {
        return true;
    }

    if (m_pauseOnBattery && IsOnBattery()) {
        return true;
    }

    return false;
}

bool PerformanceOptimizer::IsFullscreenAppActive() {
    HWND foreground = GetForegroundWindow();
    if (!foreground) return false;

    RECT rect;
    GetWindowRect(foreground, &rect);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    return (rect.right - rect.left >= screenWidth) &&
           (rect.bottom - rect.top >= screenHeight);
}

bool PerformanceOptimizer::IsOnBattery() {
    SYSTEM_POWER_STATUS status;
    if (GetSystemPowerStatus(&status)) {
        return status.ACLineStatus == 0;
    }
    return false;
}

float PerformanceOptimizer::GetGPUUsage() {
    return m_lastGpuUsage;
}

float PerformanceOptimizer::GetCPUUsage() {
    return m_lastCpuUsage;
}

int PerformanceOptimizer::GetRAMUsageMB() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return (int)(pmc.WorkingSetSize / (1024 * 1024));
    }
    return 0;
}

void PerformanceOptimizer::UpdateMetrics() {
    m_lastRamUsageMB = GetRAMUsageMB();

    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        static ULARGE_INTEGER prevIdle = {0}, prevKernel = {0}, prevUser = {0};
        ULARGE_INTEGER curIdle, curKernel, curUser;

        curIdle.LowPart = idleTime.dwLowDateTime;
        curIdle.HighPart = idleTime.dwHighDateTime;
        curKernel.LowPart = kernelTime.dwLowDateTime;
        curKernel.HighPart = kernelTime.dwHighDateTime;
        curUser.LowPart = userTime.dwLowDateTime;
        curUser.HighPart = userTime.dwHighDateTime;

        ULONGLONG sysTotal = (curKernel.QuadPart - prevKernel.QuadPart) +
                             (curUser.QuadPart - prevUser.QuadPart);
        ULONGLONG sysIdle = curIdle.QuadPart - prevIdle.QuadPart;

        if (sysTotal > 0) {
            m_lastCpuUsage = (1.0f - (float)sysIdle / (float)sysTotal) * 100.0f;
        }

        prevIdle = curIdle;
        prevKernel = curKernel;
        prevUser = curUser;
    }
}

int PerformanceOptimizer::GetOptimalQuality() {
    if (m_lastGpuUsage > 80) return 0;
    if (m_lastGpuUsage > 60) return 1;
    if (m_lastGpuUsage > 40) return 2;
    return 3;
}

void PerformanceOptimizer::ApplyOptimization() {
    if (ShouldPause()) return;

    if (m_lastRamUsageMB > m_ramLimitMB) {
        PurgeCache();
    }

    if (m_lastGpuUsage > m_gpuUsageLimit) {
        m_qualityLevel = GetOptimalQuality();
        m_frameSkipEnabled = true;
    } else {
        m_frameSkipEnabled = false;
    }
}

void PerformanceOptimizer::PurgeCache() {
    namespace fs = std::filesystem;
    fs::path cacheDir = fs::temp_directory_path() / "WallpaperEngine" / "cache";

    if (fs::exists(cacheDir)) {
        for (const auto& entry : fs::directory_iterator(cacheDir)) {
            if (entry.is_regular_file()) {
                auto lastAccess = fs::last_write_time(entry);
                auto now = fs::file_time_type::clock::now();
                auto age = std::chrono::duration_cast<std::chrono::hours>(now - lastAccess);
                if (age.count() > 24) {
                    fs::remove(entry.path());
                }
            }
        }
    }
}
