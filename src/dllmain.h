#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <windows.h>

inline const std::string CfgExtension = "ini";
inline const std::string BaseCfg = "Settings";
inline const std::string DisplayCfg = "Display";
// mINI lib operates with strings only
inline const std::string trueStr = "1";
inline const std::string falseStr = "0";

inline const std::string credits = "### Widescreen Mod (1.0) by Hypercycle\n";

inline const uintptr_t buildSunsetEntry = 0x69DDA2;
inline const std::string sunsetBuildStr = "Sunset (1.0.136.0)";

inline const uintptr_t buildDebugEntry = 0x9FC980;
inline const std::string debugBuildStr = "Debug (1.0.117.0)";

inline const char GUI_gDialogsPath[] = "system\\WS_GDial.gui"; // 8 + dot + 3
inline const char GUI_gInterfacePath[] = "system\\WS_GInterf.gui"; // 10 + dot + 3
inline const char GUI_vivPath[] = "system\\WUI.viv"; // 3 + dot + 3

inline const uint32_t gamePathsEntriesCount = 18;
inline const uint32_t gamePathsEntrySize = 0x104;

inline const uint32_t uiDesignScreenWidth = 800;
inline const uint32_t uiDesignScreenHeight = 600;
inline const uint32_t uiDesign2ScreenWidth = 640;
inline const uint32_t uiDesign2ScreenHeight = 480;
inline const uint32_t fsRes1Width = 800;
inline const uint32_t fsRes1Height = 600;
inline const uint32_t fsRes2Width = 1024;
inline const uint32_t fsRes2Height = 768;

inline const float_t uiMapSizeDamper = 0.5f;
inline const float_t uiMapSizeExampleRatio = 0.75f; // aspectRatio / AR_16_9

// Some values from the game executable

inline const float_t uiDesign_TrafficLightStyle_X = 248.0f;
inline const float_t uiDesign_TrafficLight2Style_X = 296.0f;
inline const float_t uiDesign_DragModeStyle_X = 258.0f;

// Structures

struct AspectRatioPoint
{
    float_t aspectRatio;
    float_t value;
};

extern AspectRatioPoint defaultFOVT[];
extern AspectRatioPoint interiorFOVT[];
extern AspectRatioPoint heightScaleT[];
extern AspectRatioPoint heightScaleWidescreenT[];

// FOV target values

inline const float_t AR_4_3 = 4.0f / 3.0f;
inline const float_t AR_16_9 = 16.0f / 9.0f;
inline const float_t AR_21_9 = 21.0f / 9.0f;
inline const float_t AR_32_9 = 32.0f / 9.0f;

// Address map
// First parameter is for a Sunset, second is for a Debug

namespace AddrMap
{
    namespace MCOExecutableInit
    {
        inline const uintptr_t v1_1[] = { 0x524378 , 0x68A37C };
    }
    namespace RenderViewportFOVTweaks
    {
        inline const uintptr_t vInterior[] = { 0x4A082F , 0x5813ED };
        inline const uintptr_t vDefault[] = { 0x58F6F9 , 0x7A1C3D };
    }
    namespace SkipMovies
    {
        inline const uintptr_t v[] = { 0xA383EC , 0x163D814 };
    }
    namespace GetDiskFreeSpaceFix
    {
        inline const uintptr_t v[] = { 0x83B260 , 0x20F4D7C };
    }
    namespace GPUMemorySize
    {
        inline const uintptr_t v[] = { 0x896BA0 , 0x126F1E4 };
    }
    namespace GamePaths
    {
        inline const uintptr_t regPathCheckFunc[] =       { 0x5242C1 , 0x68A1FF };
        inline const uintptr_t regPathErrorCheck_Addr[] = { 0x5242C9 , 0x68A217 };
        inline const uintptr_t regPathErrorCheck_Jmp[] =  { 0x524307 , 0x68A22D };
        //
        inline const uintptr_t rootAddr[] =               { 0xA52D48 , 0x165DCC0 };
        inline const uintptr_t rootFoldersAddr[] =        { 0x895F18 , 0x126EBCC };
        inline const uintptr_t executableName[] =         { 0x89505C , 0x126E450 };
        inline const uintptr_t executableNameAddr[] =     { 0xB2B710 , 0x20D8174 };
    }
}

//
// Util
//

std::string GetIniPath();

inline std::string iniPath = GetIniPath();
inline mINI::INIFile iniFile(iniPath);
inline mINI::INIStructure ini;

void StartTestConsole();

template<typename... Args>
// Always use c_str() for any non-char string here
void TestConsolePrint(const char* format, Args&&... args) {
    if (TestConsole) {
        std::printf(format, std::forward<Args>(args)...);
    }
}

BOOL CfgIsEnabled(std::string category, std::string name);
uint32_t CfgGetInt(std::string category, std::string name);
float_t CalcHorScale(float_t curAR, const AspectRatioPoint* res, int arrSize);
float_t GetGameMemFloat(uintptr_t addr);
uint32_t GetGameMemInt(uintptr_t addr);

//
// Common
//

void RefreshVMAspectRatio();
void RefreshUserAspectRatio(uint32_t screenWidth, uint32_t screenHeight);
void GetSystemDisplayInfo();
void RefreshScreenCfg();
void SetInitialScreenSize();
//
void SkipMovies();
void GPUMemorySizeFix();
BOOL WINAPI ReplaceGetDiskFreeSpaceA(LPCSTR lpRootPathName, LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters);
void WinCall_GetDiskFreeSpaceFix();
void DynamicGameExecutablePath();
void CheckMoviesViewScaleMultiplier();
//
BOOL CheckGameExecutableType();
void PreInitBase();