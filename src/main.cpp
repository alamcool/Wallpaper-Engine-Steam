#include <Windows.h>
#include "WallpaperEngine.h"
#include "WallpaperRenderer.h"
#include "Config.h"
#include "WorkshopClient.h"
#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    Config& config = Config::GetInstance();
    config.Load("config/settings.json");

    WorkshopClient::Initialize();
    WorkshopClient::SyncSubscriptions();

    WallpaperEngine& engine = WallpaperEngine::GetInstance();
    engine.Initialize(hInstance, config.GetDisplayMode());

    WallpaperRenderer& renderer = WallpaperRenderer::GetInstance();
    renderer.Initialize(engine.GetHWND());

    engine.SetActiveWallpaper(config.GetDefaultWallpaperPath());
    engine.SetPlaybackState(true);

    engine.Run();

    renderer.Shutdown();
    engine.Shutdown();
    WorkshopClient::Shutdown();

    return 0;
}
