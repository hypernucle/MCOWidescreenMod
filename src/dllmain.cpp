//
// A Widescreen + custom resolution helper library for the Motor City Online
// by Hypercycle
//

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <numeric>
#include <filesystem>

#include "includes/injector/injector.hpp"
#include "includes/injector/assembly.hpp"
#include "includes/mini/ini.h"

#include "dllmain.h"
#include "vars.h"
#include "build_sunset.h"
#include "build_debug.h"

// Structures

AspectRatioPoint defaultFOVT[] = {
	{AR_4_3, 85.0f},
	{AR_16_9, 102.0f},
	{AR_21_9, 118.0f},
	{AR_32_9, 136.0f}
};

AspectRatioPoint defaultIGWidescreenFOVT[] = {
	{AR_4_3, 100.0f}, // just +15.0 to everything
	{AR_16_9, 117.0f},
	{AR_21_9, 133.0f},
	{AR_32_9, 151.0f}
};

AspectRatioPoint interiorFOVT[] = {
	{AR_4_3, 80.0f},
	{AR_16_9, 97.0f},
	{AR_21_9, 113.0f},
	{AR_32_9, 129.0f}
};

AspectRatioPoint heightScaleT[] = {
	{AR_4_3, 0.75f},
	{AR_16_9, 0.5656f},
	{AR_21_9, 0.45f},
	{AR_32_9, 0.29f}
};

AspectRatioPoint heightScaleWidescreenT[] = {
	{AR_4_3, 0.50f},
	{AR_16_9, 0.375f},
	{AR_21_9, 0.28f},
	{AR_32_9, 0.185f}
};

//
// Util
//

// Relative ini-file path, prevents mINI from updating the config in a root game folder
std::string GetIniPath()
{
	char pathBuf[MAX_PATH];
	HMODULE hModule = NULL;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&GetIniPath, &hModule);
	GetModuleFileNameA(hModule, pathBuf, MAX_PATH);

	std::filesystem::path asiPath(pathBuf);

	gameFolderPath = (asiPath.parent_path().parent_path().string()) + "\\";
	std::filesystem::path iniPath = asiPath;
	iniPath.replace_extension(CfgExtension);

	return iniPath.string();
}

void StartTestConsole()
{
	AllocConsole();
	FILE* fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
}

BOOL CfgIsEnabled(std::string category, std::string name)
{
	BOOL value = false;
	std::string valueStr = ini.get(category).get(name);
	if (!valueStr.empty() && valueStr == trueStr)
	{
		value = true;
	}
	TestConsolePrint("### Config value: [%s, %s] = %d\n", category.c_str(), name.c_str(), value);
	return value;
}

uint32_t CfgGetInt(std::string category, std::string name)
{
	std::string valueStr = ini.get(category).get(name);
	if (valueStr.empty())
	{
		TestConsolePrint("!!! Config value [%s, %s] is missing.\n", category.c_str(), name.c_str());
		return 0;
	}
	size_t pos = 0;
	uint32_t value = std::stoi(valueStr, &pos);
	if (pos != valueStr.size())
	{
		TestConsolePrint("!!! Config value [%s, %s] contains non-numeric chars.\n", category.c_str(), name.c_str());
		return 0;
	}
	TestConsolePrint("### Config value: [%s, %s] = %d\n", category.c_str(), name.c_str(), value);
	return value;
}

float_t CalcHorScale(float_t curAR, const AspectRatioPoint* res, int arrSize)
{
	if (curAR <= res[0].aspectRatio) { return res[0].value; }
	if (curAR >= res[arrSize - 1].aspectRatio) { return res[arrSize - 1].value; }
	for (int i = 0; i < arrSize - 1; ++i)
	{
		const AspectRatioPoint& a = res[i];
		const AspectRatioPoint& b = res[i + 1];

		if (curAR >= a.aspectRatio && curAR <= b.aspectRatio)
		{
			float_t atanA = std::atan(a.aspectRatio);
			float_t atanB = std::atan(b.aspectRatio);
			float_t atanC = std::atan(curAR);

			float_t t = (atanC - atanA) / (atanB - atanA);
			return a.value + t * (b.value - a.value);
		}
	}
	return res[arrSize - 1].value;
}

float_t GetGameMemFloat(uintptr_t addr)
{
	return *reinterpret_cast<float_t*>(addr);
}
//
uint32_t GetGameMemInt(uintptr_t addr)
{
	return *reinterpret_cast<uint32_t*>(addr);
}

// Structures

typedef BOOL(WINAPI* GetDiskFreeSpaceA_t) (
	LPCSTR  lpRootPathName,
	LPDWORD lpSectorsPerCluster,
	LPDWORD lpBytesPerSector,
	LPDWORD lpNumberOfFreeClusters,
	LPDWORD lpTotalNumberOfClusters
);

//
// Common
//

void RefreshVMAspectRatio()
{
	uint32_t gcd = std::gcd(sysScreenWidth, sysScreenHeight);
	vmAspectWidth = sysScreenWidth / gcd;
	vmAspectHeight = sysScreenHeight / gcd;
}
//
void RefreshUserAspectRatio(uint32_t screenWidth, uint32_t screenHeight)
{
	uint32_t gcd = std::gcd(screenWidth, screenHeight);
	userAspectWidth = screenWidth / gcd;
	userAspectHeight = screenHeight / gcd;
}

void GetSystemDisplayInfo()
{
	SetProcessDPIAware();
	
	DEVMODE dm = {};
	dm.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dm);

	sysScreenWidth = dm.dmPelsWidth;
	sysScreenHeight = dm.dmPelsHeight;
	RefreshVMAspectRatio();
}

void RefreshScreenCfg()
{
	// Automatic aspect ratio, depending on the current user display video mode
	aspectRatio = (float)screenWidth / (float)screenHeight;
	uint32_t gcd = std::gcd(screenWidth, screenHeight);
	uint32_t infoAspectWidth = screenWidth / gcd;
	uint32_t infoAspectHeight = screenHeight / gcd;
	
	uiScreenSizeWidth = (int)(uiDesignScreenHeight * aspectRatio);
	uiTargetScreenWidth = (int)(screenHeight * AR_4_3);
	uiTargetMarginX = (float)(screenWidth - uiTargetScreenWidth) / 2.0f;
	uiTargetMarginX_Int = (int)uiTargetMarginX;

	// "2" aka 640x480 values are primarily used for loading screen elements
	uint32_t uiTarget2SizeWidth = (int)(uiDesign2ScreenHeight * aspectRatio);
	uiTarget2SizeWidthResizer = (float)(1.0f / uiTarget2SizeWidth);

	uiMapSizeMultiplier = (float)uiDesignScreenHeight / (float)screenHeight;

	if (!dBuild) // Element-specific calc
	{
		float_t loadingScreenTipsXOrigMargin = (float)uiDesign2ScreenWidth - GetGameMemFloat(0x893C48); // must be 40
		uiLoadingScreen_TipsX = (float)uiTarget2SizeWidth - loadingScreenTipsXOrigMargin;
		uiLoadingScreen_ProgressBarWidth = uiLoadingScreen_TipsX - 60.0f; // FIXME it works but also it's a wrong value
		//
		float_t uiCountdownXMargin = ((float)uiTarget2SizeWidth - (float)uiDesign2ScreenWidth) / 2.0f;
		uiCountdown_TrafficLightStyle_X = uiDesign_TrafficLightStyle_X + uiCountdownXMargin;
		uiCountdown_TrafficLight2Style_X = uiDesign_TrafficLight2Style_X + uiCountdownXMargin;
		uiCountdown_DragModeStyle_X = uiDesign_DragModeStyle_X + uiCountdownXMargin;
	}

	// Field of View calc
	renderInGameDefaultFOV = CalcHorScale(aspectRatio, defaultFOVT, std::size(defaultFOVT));
	renderInGameDefaultWidescreenFOV = CalcHorScale(aspectRatio, defaultIGWidescreenFOVT, std::size(defaultIGWidescreenFOVT));
	renderInGameInteriorFOV = CalcHorScale(aspectRatio, interiorFOVT, std::size(interiorFOVT));
	renderViewportHeightScale = CalcHorScale(aspectRatio, heightScaleT, std::size(heightScaleT));
	renderViewportHeightWidescreenScale = CalcHorScale(aspectRatio, heightScaleWidescreenT, std::size(heightScaleWidescreenT));

	TestConsolePrint("### Refresh of the screen configuration: %dx%d, UI viewport width: %d\n___ 3D viewport height scale: %f, aspect ratio: %f, IGDefaultFOV: %f, IGDefaultWSFOV: %f, InteriorFOV: %f, Resizer X: %f, Resizer Left Offset: %f, User Aspect Ratio: %d:%d, UI Map Size: %f\n", 
			screenWidth, screenHeight, uiScreenSizeWidth, renderViewportHeightScale, aspectRatio, renderInGameDefaultFOV, renderInGameDefaultWidescreenFOV, renderInGameInteriorFOV, uiTarget2SizeWidthResizer, uiTargetMarginX, infoAspectWidth, infoAspectHeight, uiMapSizeMultiplier);
}

void SetInitialScreenSize()
{
	// First start - set the screen size of the user's display video mode
	GetSystemDisplayInfo();
	uint32_t cfgScreenWidth = CfgGetInt(DisplayCfg, "ScreenWidth");
	uint32_t cfgScreenHeight = CfgGetInt(DisplayCfg, "ScreenHeight");

	BOOL isUserScreenSizePresent = cfgScreenWidth != 0 && cfgScreenHeight != 0;
	screenWidth = isUserScreenSizePresent ? cfgScreenWidth : sysScreenWidth;
	screenHeight = isUserScreenSizePresent ? cfgScreenHeight : sysScreenHeight;

	RefreshScreenCfg();
	// Do it only once: game is capable of updating the video modes list, however it breaks the visual modes list in some specific cases
	RefreshUserAspectRatio(screenWidth, screenHeight);
}

void SkipMovies()
{
	if (CfgIsEnabled(BaseCfg, "SkipMovies"))
	{
		injector::WriteMemory<uint8_t>(AddrMap::SkipMovies::v[dBuild], 0x1, true);
	}
}

void GPUMemorySizeFix()
{
	uint32_t gpuMemSize = CfgGetInt(BaseCfg, "GPUMemorySize");
	uint32_t gpuMemSizeBytes = gpuMemSize * 1024 * 1024;
	if (gpuMemSizeBytes > 0x7FFFFFFF)
	{
		TestConsolePrint("!!! Your custom GPU memory size is too high and will be limited to 2 GB.\n");
		gpuMemSizeBytes = 0x7FFFFFFF;
		gpuMemSize = gpuMemSizeBytes / 1024 / 1024;
	}
	if (gpuMemSizeBytes > 0)
	{
		injector::WriteMemory<uint32_t>(AddrMap::GPUMemorySize::v[dBuild], gpuMemSizeBytes, true);
		TestConsolePrint("### Forced GPU memory size is %d MB.\n", gpuMemSize);
	}
}

BOOL WINAPI ReplaceGetDiskFreeSpaceA(LPCSTR lpRootPathName, LPDWORD lpSectorsPerCluster, 
		LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters)
{
	*lpSectorsPerCluster = 8;
	*lpBytesPerSector = 512;
	*lpNumberOfFreeClusters = 2621440UL; // (DWORD)((10ULL * 1024 * 1024 * 1024) / (8 * 512))
	*lpTotalNumberOfClusters = 10485760UL; // (DWORD)((40ULL * 1024 * 1024 * 1024) / (8 * 512))
	return TRUE;
}

// A Fix for "Your HDD has less than 10MB of free space..." by always showing 10 GB out of 40 GB of free space
void WinCall_GetDiskFreeSpaceFix()
{
	injector::WriteMemory<void*>(AddrMap::GetDiskFreeSpaceFix::v[dBuild], (void*)ReplaceGetDiskFreeSpaceA, true);
	TestConsolePrint("### DLL Call replacement: GetDiskFreeSpaceA.\n");
}

// Instead of looking in the Registry, check the current relative game folder path to allow the launch of different game builds (Debug + Sunset), 
// or to enable the game folder to be moved for better portability.
void DynamicGameExecutablePath()
{
	if (!CfgIsEnabled(BaseCfg, "NonRegGameExecutablePath")) { return; }
	
	// Disable the original registry path loading
	injector::MakeNOP(AddrMap::GamePaths::regPathCheckFunc[dBuild], 5);
	// Skip error status check
	injector::MakeJMP(AddrMap::GamePaths::regPathErrorCheck_Addr[dBuild], 
			AddrMap::GamePaths::regPathErrorCheck_Jmp[dBuild], true);

	uint32_t pathAddr = AddrMap::GamePaths::rootAddr[dBuild];
	uint32_t pathFoldersAddr = AddrMap::GamePaths::rootFoldersAddr[dBuild];
	uint32_t pathAddrIndex = 0;
	
	injector::WriteMemoryRaw(pathAddr, gameFolderPath.data(), gameFolderPath.size(), true);
	pathAddr += gamePathsEntrySize;
	pathAddrIndex++;

	while (pathAddrIndex < gamePathsEntriesCount + 1)
	{
		const char* nextPathFolder = *reinterpret_cast<const char**>(pathFoldersAddr);
		std::string pathParam = gameFolderPath + nextPathFolder;
		injector::WriteMemoryRaw(pathAddr, pathParam.data(), pathParam.size(), true);
		pathAddr += gamePathsEntrySize;
		pathFoldersAddr += 0x4;
		pathAddrIndex++;
	}

	const char* executableName = reinterpret_cast<const char*>(AddrMap::GamePaths::executableName[dBuild]);
	injector::WriteMemoryRaw(AddrMap::GamePaths::executableNameAddr[dBuild], (void*)executableName, sizeof(executableName), true);
}

// Very primitive viewsize scaler, exactly like the in-game movie scaler
void CheckMoviesViewScaleMultiplier()
{
	if (screenWidth >= 3840 && screenHeight >= 2160)
	{
		movieViewScaleMultiplier = 9;
	}
	else if (screenWidth >= 2560 && screenHeight >= 1440)
	{
		movieViewScaleMultiplier = 6;
	}
	else if (screenWidth >= 1920 && screenHeight >= 1080)
	{
		movieViewScaleMultiplier = 5;
	}
	else if (screenWidth >= 1280 && screenHeight >= 720)
	{
		movieViewScaleMultiplier = 3;
	}
}

//
// Pre-boot
//

// Apply tweaks, depending on the type of game executable.
BOOL CheckGameExecutableType()
{
	uintptr_t			base		= (uintptr_t)GetModuleHandleA(NULL);
	IMAGE_DOS_HEADER*	dos			= (IMAGE_DOS_HEADER*)(base);
	IMAGE_NT_HEADERS*	nt			= (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
	uintptr_t			entryAddr	= base + nt->OptionalHeader.AddressOfEntryPoint + (0x400000 - base);
	
	// 1.0.136.0
	if (entryAddr == buildSunsetEntry)
	{
		dBuild = false;
		return true;
	}
	// 1.0.117.0
	else if (entryAddr == buildDebugEntry)
	{
		dBuild = true;
		return true;
	}
	return false;
}

void PreInitBase()
{
	if (!CheckGameExecutableType()) { return; };
	BOOL isIniReady = iniFile.read(ini);

	if (!CfgIsEnabled(BaseCfg, "EnableASI")) { return; }
	if (CfgIsEnabled(BaseCfg, "EnableTestConsole"))
	{
		TestConsole = true;
		StartTestConsole();
		TestConsolePrint(credits.c_str());
		TestConsolePrint("### Game build type: %s, ini-file check: %d\n", 
				dBuild ? debugBuildStr.c_str() : sunsetBuildStr.c_str(), isIniReady ? 1 : 0);
	}

	DynamicGameExecutablePath();

	// This part is only reachable after NPS auth is successful, or when NoFront mode is active
	dBuild ? InitDebug() : InitSunset();
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		PreInitBase();
	}
	return TRUE;
}
