#pragma once
#define WIN32_LEAN_AND_MEAN
#include <numeric>

#include "includes/injector/injector.hpp"
#include "includes/injector/assembly.hpp"
#include "includes/mini/ini.h"

#include "dllmain.h"
#include "vars.h"

//

void BldDebug_ForceFrontEndResolution()
{
	// ??? 1
	injector::WriteMemory<uint16_t>(0x5B7546, screenHeight, true);
	injector::WriteMemory<uint16_t>(0x5B754B, screenWidth, true);

	// Must be a movie playback?
	injector::WriteMemory<uint16_t>(0x55D110, screenHeight, true);
	injector::WriteMemory<uint16_t>(0x55D115, screenWidth, true);

	// Loading Screens
	injector::WriteMemory<uint16_t>(0x6B1698, screenHeight, true);
	injector::WriteMemory<uint16_t>(0x6B169D, screenWidth, true);

	// ??? 3
	injector::WriteMemory<uint16_t>(0x6B2147, screenHeight, true);
	injector::WriteMemory<uint16_t>(0x6B214C, screenWidth, true);

	// Main FrontEnd
	injector::WriteMemory<uint16_t>(0x7F5FFF, screenHeight, true);
	injector::WriteMemory<uint16_t>(0x7F6004, screenWidth, true);

	// Prevent the game from updating the screen size on the race loading (we can't load the menus properly here so...)
	injector::MakeNOP(0x685730, 5);
}

void BldDebug_RenderViewportFOVTweaks()
{
	if (!CfgIsEnabled(DisplayCfg, "EnableFOVScaling")) { return; }
	
	injector::WriteMemory<float_t>(0x5813ED, renderInGameInteriorFOV, true);
	injector::WriteMemory<float_t>(0x7A1C3D, renderInGameDefaultFOV, true);
	injector::WriteMemory<float_t>(0x7A1C34, renderInGameDefaultWidescreenFOV, true);
}

// Prevent the game from rendering unnecessary parts of the screen at the top and bottom.
// This is only visible when the in-game Screen Size option is set to less than 100%.
// Widescreen screen size is also re-used by the game for a 3D loading screens.
void BldDebug_ScreenSize_HeightScaleFix()
{
	injector::WriteMemory<float_t>(0x73CB1C, renderViewportHeightWidescreenScale, true);
	injector::WriteMemory<float_t>(0x73CB25, renderViewportHeightScale, true);
}

// Unfinished - only for the GUI debug mode
void BldDebug_UIScalingTweaks()
{
	if (!CfgIsEnabled(DisplayCfg, "EnableUIScalingTest")) { return; }

	// During display mode change
	injector::WriteMemory<uint32_t>(0x75F07F, uiScreenSizeWidth, true);
	// In-Game
	injector::WriteMemory<uint32_t>(0x73E94F, uiScreenSizeWidth, true);
	// FrontEnd
	injector::WriteMemory<uint32_t>(0x7F612D, uiScreenSizeWidth, true);
}

// Ignore "togglewindowtext.txt" - it sometimes blocks the Alt + Enter window mode toggle
void BldDebug_DisableFullscreenSafetyCheck()
{
	if (!CfgIsEnabled(BaseCfg, "DisableFullscreenSafetyCheck")) { return; }

	// Disable the pop-up message box on game boot
	injector::MakeNOP(0x6B1511, 9);
	injector::MakeJMP(0x6B1511, 0x6B157C);

	// Skip another instance of this safety check during the Alt + Enter attempt
	injector::MakeNOP(0x429EB5, 7);
	injector::MakeJMP(0x429EB5, 0x429F2D);
}

uintptr_t BldDebug_ForceWindowedModeOnInit_RetPtr = 0xA4911A;
__declspec(naked) void BldDebug_ForceWindowedModeOnInit_asmPart()
{
	_asm
	{
		mov EAX, 0x1
		jmp BldDebug_ForceWindowedModeOnInit_RetPtr
	}
}

// By default, the game forces the window into fullscreen mode when it starts up, regardless of your in-game settings.
// This fix enables the game to render the initial black screen in a window when it runs in Windowed mode.
void BldDebug_DisableForcedInitFullscreen()
{
	if (!CfgIsEnabled(BaseCfg, "DisableForcedInitFullscreen")) { return; }

	injector::MakeNOP(0xA49120, 5);

	if (CfgIsEnabled(BaseCfg, "ForceWindowedModeOnInit"))
	{
		// Always start the game in Windowed mode, useful in case of video mode game crashes
		injector::MakeNOP(0xA49115, 5);
		injector::MakeJMP(0xA49115, BldDebug_ForceWindowedModeOnInit_asmPart, true);
	}
	else
	{
		// Read the real Windowed Mode toggle value
		injector::WriteMemory<uint32_t>(0xA49116, 0x1409BC8, true);
	}
}

uintptr_t BldDebug_IncreaseMoviesViewScale_CalcLetterbox_RetPtr = 0x5B6D43;
__declspec(naked) void BldDebug_IncreaseMoviesViewScale_CalcLetterbox()
{
	_asm
	{
		// EAX - movie width, EDX - movie height
		// Left offset
		imul EAX, dword ptr [movieViewScaleMultiplier] // width + multiplier
		mov ECX, dword ptr [screenWidth]
		sub ECX, EAX
		sar ECX, 0x1
		mov dword ptr [EBP - 0x4C], ECX
		// Top offset
		imul EDX, dword ptr [movieViewScaleMultiplier]
		mov EAX, dword ptr [screenHeight]
		sub EAX, EDX
		sar EAX, 0x1
		mov dword ptr [EBP - 0x50], EAX
		jmp BldDebug_IncreaseMoviesViewScale_CalcLetterbox_RetPtr
	}
}

// Bigger screen size for the movies. 
// Please note that it won't be an exact match for the size of an actual screen, due to the way the game scales movie playback.
void BldDebug_IncreaseMoviesViewScale()
{
	if (!CfgIsEnabled(BaseCfg, "IncreaseMovieViewScale")) { return; }

	CheckMoviesViewScaleMultiplier();

	injector::MakeNOP(0x5B6D1D, 38); //
	injector::MakeJMP(0x5B6D1D, BldDebug_IncreaseMoviesViewScale_CalcLetterbox, true); //

	injector::WriteMemory<uint32_t>(0x5B58CE, movieViewScaleMultiplier, true);
}

// Unfinished - need to find more of it
// Note: the game will still produce the logs in case of game exceptions.
void BldDebug_DisableDebugLogs()
{
	if (!CfgIsEnabled(BaseCfg, "DisableDebugLogs")) { return; }

	// "Trace%.txt"
	injector::MakeNOP(0x9008AA, 5);
}

//
// Loading
//

// Do stuff when we are sure that the main MCO.exe has booted instead of the NPS login executable/launcher
void InitDebugBase()
{
	if (CfgIsEnabled(DisplayCfg, "EnableDisplayTweaks"))
	{
		SetInitialScreenSize();
		BldDebug_ForceFrontEndResolution();
		BldDebug_ScreenSize_HeightScaleFix();
		BldDebug_RenderViewportFOVTweaks();
		BldDebug_UIScalingTweaks();
	}
	BldDebug_DisableDebugLogs();
	SkipMovies();
	BldDebug_IncreaseMoviesViewScale();
	BldDebug_DisableForcedInitFullscreen();
	BldDebug_DisableFullscreenSafetyCheck();
	GPUMemorySizeFix(); 
	WinCall_GetDiskFreeSpaceFix(); 
}

uintptr_t mcoExecutableInit_Debug_RetPtr = 0x68A384;
__declspec(naked) void MCOExecutableInit_Debug_asmPart()
{
	_asm
	{
		call InitDebugBase
		push 0x6A // orig
		lea ECX, [EBP - 0x118] // orig
		jmp mcoExecutableInit_Debug_RetPtr
	}
}

void InitDebug()
{
	injector::MakeNOP(0x68A37C, 8);
	injector::MakeJMP(0x68A37C, MCOExecutableInit_Debug_asmPart);
}