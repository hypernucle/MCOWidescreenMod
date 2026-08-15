#pragma once
#define WIN32_LEAN_AND_MEAN
#include <numeric>

#include "includes/injector/injector.hpp"
#include "includes/injector/assembly.hpp"
#include "includes/mini/ini.h"

#include "dllmain.h"
#include "vars.h"

//

__declspec(naked) void ForceFrontEndResolution_applyCustomRes()
{
	_asm
	{
		pop EAX // return addr
		push dword ptr [screenHeight]
		push dword ptr [screenWidth]
		push EAX
		ret
	}
}

void BldSunset_ForceFrontEndResolution()
{
	// ??? 1
	injector::MakeNOP(0x4B6881, 10);
	injector::MakeCALL(0x4B6881, ForceFrontEndResolution_applyCustomRes);

	// Movie Playback
	injector::MakeNOP(0x4B69CD, 10);
	injector::MakeCALL(0x4B69CD, ForceFrontEndResolution_applyCustomRes);

	// ??? 2
	injector::MakeNOP(0x48B28B, 10);
	injector::MakeCALL(0x48B28B, ForceFrontEndResolution_applyCustomRes);

	// Loading Screens + Boot
	injector::MakeNOP(0x534AFA, 10);
	injector::MakeCALL(0x534AFA, ForceFrontEndResolution_applyCustomRes);

	// Loading Screens + Boot #2
	injector::MakeNOP(0x535099, 10);
	injector::MakeCALL(0x535099, ForceFrontEndResolution_applyCustomRes);

	// Main FrontEnd
	injector::MakeNOP(0x5C46F2, 10);
	injector::MakeCALL(0x5C46F2, ForceFrontEndResolution_applyCustomRes);
}

void RefreshScreenCfgInGame()
{
	// Get the current screen size
	screenWidth = GetGameMemInt(0xAA4510);
	screenHeight = GetGameMemInt(0xAA4514);
	RefreshScreenCfg();
}

uintptr_t screenCfgRefreshHook_RetPtr = 0x56741D;
uintptr_t videoModeIndex = 0x89937C;
uintptr_t videoModeMenuPrevIndex = 0x899384;
__declspec(naked) void ScreenCfgRefreshHook_asmPart()
{
	_asm
	{
		pushad			// safety in case of non-game code
		sub ESP, 0x4	// and to prevent issues with the size of the game window
		fnstcw word ptr [ESP]
		call RefreshScreenCfgInGame
		fldcw word ptr [ESP]
		add ESP, 0x4
		popad
		//
		mov ECX, dword ptr [videoModeMenuPrevIndex]
		mov dword ptr [ECX], EBX
		mov ECX, dword ptr [videoModeIndex]
		mov dword ptr [ECX], EBX // orig
		jmp screenCfgRefreshHook_RetPtr
	}
}

// Keep our current resolution for the next game boot
void ScreenCfgRefreshHook_UpdateIni()
{
	ini[DisplayCfg]["ScreenWidth"] = std::to_string(screenWidth);
	ini[DisplayCfg]["ScreenHeight"] = std::to_string(screenHeight);

	if (!iniFile.write(ini))
	{
		TestConsolePrint("!!! Ini-file is unable to be updated with the new screen size.\n");
	}
}

uintptr_t screenCfgRefreshHook_UpdateIni_RetPtr = 0x61AE5D;
__declspec(naked) void ScreenCfgRefreshHook_UpdateIni_asmPart()
{
	_asm
	{
		pushad			// safety in case of non-game code
		sub ESP, 0x4
		fnstcw word ptr [ESP]
		call ScreenCfgRefreshHook_UpdateIni
		fldcw word ptr [ESP]
		add ESP, 0x4
		popad
		//
		jmp screenCfgRefreshHook_UpdateIni_RetPtr
	}
}

void ScreenCfgRefreshHook()
{
	// Allow user to choose the same chosen resolution in the menu at any time
	injector::MakeNOP(0x61ACE2, 12);
	injector::MakeNOP(0x61ACF6, 8);
	injector::MakeNOP(0x61AD19, 6);

	// Disable the return to the old FrontEnd resolution after applying a new one with ScreenRez pop-up
	injector::MakeNOP(0x61AE58, 5);
	injector::MakeJMP(0x61AE58, ScreenCfgRefreshHook_UpdateIni_asmPart);

	injector::MakeNOP(0x567417, 6);
	injector::MakeJMP(0x567417, ScreenCfgRefreshHook_asmPart);
}

uintptr_t videoModeAspectFilter_SkipPtr = 0x566D31;
uintptr_t videoModeAspectFilter_PassPtr = 0x566C9B;
__declspec(naked) void VideoModeAspectFilter_asmPart()
{
	_asm
	{
		// fail-safe 800x600
		cmp EBX, dword ptr [fsRes1Width]
		jnz check_fsres2
		mov EAX, dword ptr [ESI + 0x4]
		cmp EAX, dword ptr [fsRes1Height]
		jz pass_check
		// fail-safe 1024x768
		check_fsres2:
		cmp EBX, dword ptr [fsRes2Width]
		jnz check_ar_sys
		mov EAX, dword ptr [ESI + 0x4]
		cmp EAX, dword ptr [fsRes2Height]
		jz pass_check
		// system VM aspect ratio
		check_ar_sys:
		mov EAX, dword ptr [ESI + 0x4]
		imul EAX, dword ptr [vmAspectWidth]
		mov EDX, EBX
		imul EDX, dword ptr [vmAspectHeight]
		cmp EAX, EDX
		jz pass_check
		// user VM aspect ratio
		mov EAX, dword ptr [ESI + 0x4]
		imul EAX, dword ptr [userAspectWidth]
		mov EDX, EBX
		imul EDX, dword ptr [userAspectHeight]
		cmp EAX, EDX
		jnz skip_mode
		// pass: proceed to other checks
		pass_check:
		jmp videoModeAspectFilter_PassPtr
		// skip: try another resolution
		skip_mode:
		jmp videoModeAspectFilter_SkipPtr
	}
}

// Get only a set of video modes that follow the chosen aspect ratio.
// The resolution list is being loaded from a render "thrash" module, eg. dx8z.dll
// Includes a fail-safe 800x600 + 1024x768
// + the aspect ratio from a currently loaded screen resolution (in case of different Window + Desktop resolutions).
void VideoModeAspectFilter()
{
	uint32_t vmCfgAspectWidth = CfgGetInt(DisplayCfg, "VideoModeAspectWidth");
	uint32_t vmCfgAspectHeight = CfgGetInt(DisplayCfg, "VideoModeAspectHeight");
	if (vmCfgAspectWidth != 0 && vmCfgAspectHeight != 0)
	{
		vmAspectWidth = vmCfgAspectWidth;
		vmAspectHeight = vmCfgAspectHeight;
	}

	injector::MakeNOP(0x566C86, 21);
	injector::MakeJMP(0x566C86, VideoModeAspectFilter_asmPart);
}

// Prevent the game from rendering unnecessary parts of the screen at the top and bottom.
// This is only visible when the in-game Screen Size option is set to less than 100%.
// Widescreen screen size is also re-used by the game for a 3D loading screens.
void BldSunset_ScreenSize_HeightScaleFix()
{
	injector::WriteMemory<uintptr_t>(0x56612D, (uintptr_t)&renderViewportHeightWidescreenScale, true);
	injector::WriteMemory<uintptr_t>(0x566135, (uintptr_t)&renderViewportHeightScale, true);
}

uintptr_t RenderViewportFOVTweak_Interior_RetPtr = 0x4A0833;
__declspec(naked) void RenderViewportFOVTweak_Interior_asmPart()
{
	_asm
	{
		push dword ptr [renderInGameInteriorFOV]
		jmp RenderViewportFOVTweak_Interior_RetPtr
	}
}

uintptr_t RenderViewportFOVTweak_Driving_RetPtr = 0x58F6FD;
__declspec(naked) void RenderViewportFOVTweak_Driving_asmPart()
{
	_asm
	{
		// is Options_Render_Widescreen = 1?
		mov EAX, dword ptr [renderInGameDefaultWidescreenFOV]
		jnz applyFOV
		mov EAX, dword ptr [renderInGameDefaultFOV]
		applyFOV:
		mov dword ptr [EBP - 0x4], EAX
		jmp RenderViewportFOVTweak_Driving_RetPtr
	}
}

void BldSunset_RenderViewportFOVTweaks()
{
	if (!CfgIsEnabled(DisplayCfg, "EnableFOVScaling")) { return; }

	injector::MakeNOP(0x4A082E, 5);
	injector::MakeJMP(0x4A082E, RenderViewportFOVTweak_Interior_asmPart);
	injector::MakeNOP(0x58F6ED, 16);
	injector::MakeJMP(0x58F6ED, RenderViewportFOVTweak_Driving_asmPart);
}

uintptr_t UIScalingTweaks_AddMargin1_RetPtr = 0x49F9DF;
__declspec(naked) void UIScalingTweaks_AddMargin1_asmPart()
{
	_asm
	{
		fstp dword ptr [EAX - 0x8] // orig
		fld dword ptr [EAX - 0x8]
		fadd dword ptr [uiTargetMarginX]
		fstp dword ptr [EAX - 0x8]
		fild dword ptr [uiTargetScreenWidth] // use our screen size instead
		jmp UIScalingTweaks_AddMargin1_RetPtr
	}
}

uintptr_t UIScalingTweaks_AddMargin2_RetPtr = 0x49F9F6;
__declspec(naked) void UIScalingTweaks_AddMargin2_asmPart()
{
	_asm
	{
		fstp dword ptr [EAX] // orig
		fld dword ptr [EAX]
		fadd dword ptr [uiTargetMarginX]
		fstp dword ptr [EAX]
		fild dword ptr [screenHeight] // use our screen size instead
		jmp UIScalingTweaks_AddMargin2_RetPtr
	}
}

uintptr_t UIScalingTweaks_UITargetSize_Update_RetPtr = 0x575F61;
__declspec(naked) void UIScalingTweaks_UITargetSize_Update_asmPart()
{
	_asm
	{
		mov EAX, dword ptr [uiScreenSizeWidth]
		mov dword ptr [EBP - 0x18], EAX
		jmp UIScalingTweaks_UITargetSize_Update_RetPtr
	}
}

uintptr_t UIScalingTweaks_UITargetSize_InGame_RetPtr = 0x56758F;
__declspec(naked) void UIScalingTweaks_UITargetSize_InGame_asmPart()
{
	_asm
	{
		mov EAX, dword ptr [uiScreenSizeWidth]
		mov dword ptr [EBP - 0x28], EAX
		jmp UIScalingTweaks_UITargetSize_InGame_RetPtr
	}
}

uintptr_t UIScalingTweaks_UITargetSize_FrontEnd_RetPtr = 0x5C4805;
__declspec(naked) void UIScalingTweaks_UITargetSize_FrontEnd_asmPart()
{
	_asm
	{
		mov EAX, dword ptr [uiScreenSizeWidth]
		mov dword ptr [EBP - 0x2C], EAX
		jmp UIScalingTweaks_UITargetSize_FrontEnd_RetPtr
	}
}

//

uintptr_t UIScalingTweaks_LoadingScreen_AddXOffset1_RetPtr = 0x4B4442;
__declspec(naked) void UIScalingTweaks_LoadingScreen_AddXOffset1_asmPart()
{
	_asm
	{
		mov EDX, dword ptr [EBP - 0x28] // orig
		add EDX, dword ptr [uiTargetMarginX_Int]
		push EAX // orig
		mov EAX, dword ptr [ESI] // orig
		jmp UIScalingTweaks_LoadingScreen_AddXOffset1_RetPtr
	}
}

uintptr_t UIScalingTweaks_LoadingScreen_AddXOffset2_RetPtr = 0x4B45C1;
__declspec(naked) void UIScalingTweaks_LoadingScreen_AddXOffset2_asmPart()
{
	_asm
	{
		mov EDX, dword ptr [EBP - 0x40] // orig
		add EDX, dword ptr [uiTargetMarginX_Int]
		push EAX // orig
		mov EAX, dword ptr [ESI] // orig
		jmp UIScalingTweaks_LoadingScreen_AddXOffset2_RetPtr
	}
}

uintptr_t UIScalingTweaks_LoadingScreen_AddXOffset3_RetPtr = 0x4B4740;
__declspec(naked) void UIScalingTweaks_LoadingScreen_AddXOffset3_asmPart()
{
	_asm
	{
		mov EDX, dword ptr [EBP - 0x30] // orig
		add EDX, dword ptr [uiTargetMarginX_Int]
		push EAX // orig
		mov EAX, dword ptr [ESI] // orig
		jmp UIScalingTweaks_LoadingScreen_AddXOffset3_RetPtr
	}
}

//

uintptr_t UIScalingTweaks_TitleLoadingScreen_AddXOffset1_RetPtr = 0x4B4EEE;
__declspec(naked) void UIScalingTweaks_TitleLoadingScreen_AddXOffset1_asmPart()
{
	_asm
	{
		mov EAX, dword ptr [EBP - 0x44] // orig
		add EAX, dword ptr [uiTargetMarginX_Int]
		push ECX // orig
		mov ECX, dword ptr [ESI] // orig
		jmp UIScalingTweaks_TitleLoadingScreen_AddXOffset1_RetPtr
	}
}

uintptr_t UIScalingTweaks_TitleLoadingScreen_AddXOffset2_RetPtr = 0x4B4FCC;
__declspec(naked) void UIScalingTweaks_TitleLoadingScreen_AddXOffset2_asmPart()
{
	_asm
	{
		mov EAX, dword ptr [EBP - 0x24] // orig
		add EAX, dword ptr [uiTargetMarginX_Int]
		push ECX // orig
		mov ECX, dword ptr [ESI] // orig
		jmp UIScalingTweaks_TitleLoadingScreen_AddXOffset2_RetPtr
	}
}

uintptr_t UIScalingTweaks_TitleLoadingScreen_AddXOffset3_RetPtr = 0x4B50AA;
__declspec(naked) void UIScalingTweaks_TitleLoadingScreen_AddXOffset3_asmPart()
{
	_asm
	{
		mov EAX, dword ptr [EBP - 0x24] // orig
		add EAX, dword ptr [uiTargetMarginX_Int]
		push ECX // orig
		mov ECX, dword ptr [ESI] // orig
		jmp UIScalingTweaks_TitleLoadingScreen_AddXOffset3_RetPtr
	}
}

//

uintptr_t UIScalingTweaks_StartCountdown_AddMargin_RetPtr = 0x4A1CB2;
__declspec(naked) void UIScalingTweaks_StartCountdown_AddMargin_asmPart()
{
	_asm
	{
		mov ECX, dword ptr [uiCountdown_TrafficLightStyle_X]
		mov dword ptr [EBP - 0x4], ECX
		jmp UIScalingTweaks_StartCountdown_AddMargin_RetPtr
	}
}

uintptr_t UIScalingTweaks_StartCountdown_AddMargin2_RetPtr = 0x4A1CCC;
__declspec(naked) void UIScalingTweaks_StartCountdown_AddMargin2_asmPart()
{
	_asm
	{
		mov ECX, dword ptr [uiCountdown_TrafficLight2Style_X]
		mov dword ptr [EBP - 0x4], ECX
		jmp UIScalingTweaks_StartCountdown_AddMargin2_RetPtr
	}
}

__declspec(naked) void UIScalingTweaks_StartCountdown_AddMargin3()
{
	_asm
	{
		pop EAX // return addr
		push dword ptr [uiCountdown_DragModeStyle_X]
		push EAX
		ret
	}
}

//

// GUI in this game is using it's own screen size bounds, which gets resized to fit the real screen
void BldSunset_UIScalingTweaks()
{
	if (!CfgIsEnabled(DisplayCfg, "EnableUIScaling")) { return; }

	// During display mode change
	injector::MakeNOP(0x575F5A, 7);
	injector::MakeJMP(0x575F5A, UIScalingTweaks_UITargetSize_Update_asmPart, true);
	// In-Game
	injector::MakeNOP(0x567588, 7);
	injector::MakeJMP(0x567588, UIScalingTweaks_UITargetSize_InGame_asmPart, true);
	// FrontEnd
	injector::MakeNOP(0x5C47FE, 7);
	injector::MakeJMP(0x5C47FE, UIScalingTweaks_UITargetSize_FrontEnd_asmPart, true);

	// UI objects scale & position
	injector::WriteMemory<uintptr_t>(0x49F888, (uintptr_t)&uiTargetScreenWidth, true);
	injector::WriteMemory<uintptr_t>(0x49F9C3, (uintptr_t)&uiTargetScreenWidth, true);

	injector::MakeNOP(0x49F9D6, 9);
	injector::MakeJMP(0x49F9D6, UIScalingTweaks_AddMargin1_asmPart, true);
	injector::MakeNOP(0x49F9EE, 8);
	injector::MakeJMP(0x49F9EE, UIScalingTweaks_AddMargin2_asmPart, true);

	// Mini-map
	// Player dot size
	injector::WriteMemory<uintptr_t>(0x4A4277, (uintptr_t)&uiTarget2SizeWidthResizer, true);
	injector::WriteMemory<uintptr_t>(0x4A428F, (uintptr_t)&uiTarget2SizeWidthResizer, true);
	// Map Size multiplier
	injector::WriteMemory<uintptr_t>(0x4A4105, (uintptr_t)&uiMapSizeMultiplier, true);
	injector::WriteMemory<uintptr_t>(0x4A4426, (uintptr_t)&uiMapSizeMultiplier, true);

	// Start Countdown
	// Traffic lights
	injector::MakeNOP(0x4A1CAB, 7);
	injector::MakeJMP(0x4A1CAB, UIScalingTweaks_StartCountdown_AddMargin_asmPart, true);
	injector::MakeNOP(0x4A1CC5, 7);
	injector::MakeJMP(0x4A1CC5, UIScalingTweaks_StartCountdown_AddMargin2_asmPart, true);
	// Drag mode lights
	injector::MakeNOP(0x4A1B73, 5);
	injector::MakeCALL(0x4A1B73, UIScalingTweaks_StartCountdown_AddMargin3, true);
	injector::MakeNOP(0x4A1BA9, 5);
	injector::MakeCALL(0x4A1BA9, UIScalingTweaks_StartCountdown_AddMargin3, true);
	injector::MakeNOP(0x4A1BDF, 5);
	injector::MakeCALL(0x4A1BDF, UIScalingTweaks_StartCountdown_AddMargin3, true);
	injector::MakeNOP(0x4A1C15, 5);
	injector::MakeCALL(0x4A1C15, UIScalingTweaks_StartCountdown_AddMargin3, true);
	injector::MakeNOP(0x4A1C4B, 5);
	injector::MakeCALL(0x4A1C4B, UIScalingTweaks_StartCountdown_AddMargin3, true);

	// Menu > Loading Screen (banner)
	// Layout width resizer
	injector::WriteMemory<uintptr_t>(0x4B436F, (uintptr_t)&uiTarget2SizeWidthResizer, true);
	// Layout width resizer (nicknames)
	injector::WriteMemory<uintptr_t>(0x4B4095, (uintptr_t)&uiTarget2SizeWidthResizer, true);
	injector::WriteMemory<uintptr_t>(0x4B4133, (uintptr_t)&uiTarget2SizeWidthResizer, true);
	// Add X offset for the letterbox effect
	injector::MakeNOP(0x4B443C, 6);
	injector::MakeJMP(0x4B443C, UIScalingTweaks_LoadingScreen_AddXOffset1_asmPart, true);
	injector::MakeNOP(0x4B45BB, 6);
	injector::MakeJMP(0x4B45BB, UIScalingTweaks_LoadingScreen_AddXOffset2_asmPart, true);
	injector::MakeNOP(0x4B473A, 6);
	injector::MakeJMP(0x4B473A, UIScalingTweaks_LoadingScreen_AddXOffset3_asmPart, true);

	// Menu > Loading Screen (3D camera + tips)
	// Layout width resizer
	injector::WriteMemory<uintptr_t>(0x55F130, (uintptr_t)&uiTarget2SizeWidthResizer, true);
	// Layout width resizer (nicknames)
	injector::WriteMemory<uintptr_t>(0x4B51D6, (uintptr_t)&uiTarget2SizeWidthResizer, true);
	// Tips X position for the upper & lower text labels
	injector::WriteMemory<uintptr_t>(0x55F200, (uintptr_t)&uiLoadingScreen_TipsX, true);
	injector::WriteMemory<uintptr_t>(0x55F26E, (uintptr_t)&uiLoadingScreen_TipsX, true);
	// Player progress bar width
	injector::WriteMemory<uintptr_t>(0x4B5274, (uintptr_t)&uiTarget2SizeWidthResizer, true);
	injector::WriteMemory<uintptr_t>(0x4B4988, (uintptr_t)&uiTarget2SizeWidthResizer, true);
	injector::WriteMemory<uintptr_t>(0x4B49FA, (uintptr_t)&uiLoadingScreen_ProgressBarWidth, true);

	// Racing > Loading Screen (or title screen)
	// Layout width resizer
	injector::WriteMemory<uintptr_t>(0x4B4E19, (uintptr_t)&uiTarget2SizeWidthResizer, true);
	// Add X offset for the letterbox effect
	injector::MakeNOP(0x4B4EE8, 6);
	injector::MakeJMP(0x4B4EE8, UIScalingTweaks_TitleLoadingScreen_AddXOffset1_asmPart, true);
	injector::MakeNOP(0x4B4FC6, 6);
	injector::MakeJMP(0x4B4FC6, UIScalingTweaks_TitleLoadingScreen_AddXOffset2_asmPart, true);
	injector::MakeNOP(0x4B50A4, 6);
	injector::MakeJMP(0x4B50A4, UIScalingTweaks_TitleLoadingScreen_AddXOffset3_asmPart, true);
}

// Load the GUI files under different names to avoid overwriting the original files.
void BldSunset_GUIFilePaths()
{
	if (!CfgIsEnabled(DisplayCfg, "ReplaceGUIPaths")) { return; }

	injector::WriteMemoryRaw(0x8B33A8, (void*)GUI_gDialogsPath, sizeof(GUI_gDialogsPath), true);
	injector::WriteMemoryRaw(0x8B5404, (void*)GUI_gInterfacePath, sizeof(GUI_gInterfacePath), true);
	injector::WriteMemoryRaw(0x8D2C98, (void*)GUI_gInterfacePath, sizeof(GUI_gInterfacePath), true);
	injector::WriteMemoryRaw(0x8B541C, (void*)GUI_vivPath, sizeof(GUI_vivPath), true);
}

// Ignore "togglewindowtext.txt" - it sometimes blocks the Alt + Enter window mode toggle
void BldSunset_DisableFullscreenSafetyCheck()
{
	if (!CfgIsEnabled(BaseCfg, "DisableFullscreenSafetyCheck")) { return; }

	// Disable the pop-up message box on game boot
	injector::MakeNOP(0x534783, 7);
	injector::MakeJMP(0x534783, 0x5347BA);

	// Skip another instance of this safety check during the Alt + Enter attempt
	injector::MakeNOP(0x406CCB, 6);
	injector::MakeJMP(0x406CCB, 0x406CF6);
}

// !!! Unfinished (The game lacks of in-game race state information)
void NoFrontMode()
{
	if (!CfgIsEnabled(BaseCfg, "NoFrontMode")) { return; }

	// NoFront mode
	injector::WriteMemory<uint8_t>(0xA3842C, 0x1, true);

	// Skip NPS MCO executable call
	injector::WriteMemory<uint16_t>(0x524338, 0x3EEB, true);

	// Skip some online-check
	injector::WriteMemory<uint8_t>(0x5C4E31, 0x9, true);
}

uintptr_t BldSunset_ForceWindowedModeOnInit_RetPtr = 0x6D0D7A;
__declspec(naked) void BldSunset_ForceWindowedModeOnInit_asmPart()
{
	_asm
	{
		mov EAX, 0x1
		jmp BldSunset_ForceWindowedModeOnInit_RetPtr
	}
}

// By default, the game forces the window into fullscreen mode when it starts up, regardless of your in-game settings.
// This fix enables the game to render the initial black screen in a window when it runs in Windowed mode.
void BldSunset_DisableForcedInitFullscreen()
{
	if (!CfgIsEnabled(BaseCfg, "DisableForcedInitFullscreen")) { return; }

	injector::MakeNOP(0x6D0D80, 5);
	
	if (CfgIsEnabled(BaseCfg, "ForceWindowedModeOnInit")) 
	{
		// Always start the game in Windowed mode, useful in case of video mode game crashes
		injector::MakeNOP(0x6D0D75, 5);
		injector::MakeJMP(0x6D0D75, BldSunset_ForceWindowedModeOnInit_asmPart, true);
	}
	else
	{
		// Read the real Windowed Mode toggle value
		injector::WriteMemory<uint32_t>(0x6D0D76, 0x9ABA08, true);
	}
}

uintptr_t BldSunset_IncreaseMoviesViewScale_CalcLetterbox_RetPtr = 0x4B5D9F;
__declspec(naked) void BldSunset_IncreaseMoviesViewScale_CalcLetterbox()
{
	_asm
	{
		// EDI - movie width, ESI - movie height
		// Left offset
		mov EDX, EDI
		imul EDX, dword ptr [movieViewScaleMultiplier] // width + multiplier
		mov ECX, dword ptr [screenWidth]
		sub ECX, EDX
		sar ECX, 0x1
		// Top offset
		mov EDX, ESI
		imul EDX, dword ptr [movieViewScaleMultiplier]
		mov EAX, dword ptr [screenHeight]
		sub EAX, EDX
		sar EAX, 0x1
		jmp BldSunset_IncreaseMoviesViewScale_CalcLetterbox_RetPtr
	}
}

uintptr_t BldSunset_IncreaseMoviesViewScale_RetPtr = 0x4B5476;
__declspec(naked) void BldSunset_IncreaseMoviesViewScale_asmPart()
{
	_asm
	{
		mov EBX, [movieViewScaleMultiplier]
		jmp BldSunset_IncreaseMoviesViewScale_RetPtr
	}
}

// Bigger screen size for the movies. 
// Please note that it won't be an exact match for the size of an actual screen, due to the way the game scales movie playback.
void BldSunset_IncreaseMoviesViewScale()
{
	if (!CfgIsEnabled(BaseCfg, "IncreaseMovieViewScale")) { return; }

	CheckMoviesViewScaleMultiplier();

	injector::MakeNOP(0x4B5D8A, 21);
	injector::MakeJMP(0x4B5D8A, BldSunset_IncreaseMoviesViewScale_CalcLetterbox, true);

	injector::MakeNOP(0x4B546F, 7);
	injector::MakeJMP(0x4B546F, BldSunset_IncreaseMoviesViewScale_asmPart, true);
	injector::MakeNOP(0x4B547B, 3);
	injector::MakeNOP(0x4B5485, 1);
}

//
// Loading
//

// Do stuff when we are sure that the main MCO.exe has booted instead of the NPS login executable/launcher
void InitSunsetBase()
{
	if (CfgIsEnabled(DisplayCfg, "EnableDisplayTweaks"))
	{
		SetInitialScreenSize();
		ScreenCfgRefreshHook();
		BldSunset_ForceFrontEndResolution();
		VideoModeAspectFilter();
		BldSunset_ScreenSize_HeightScaleFix();
		BldSunset_RenderViewportFOVTweaks();
		BldSunset_UIScalingTweaks();
		BldSunset_GUIFilePaths();
	}
	SkipMovies(); // Common, dllmain
	BldSunset_IncreaseMoviesViewScale();
	BldSunset_DisableForcedInitFullscreen();
	BldSunset_DisableFullscreenSafetyCheck();
	GPUMemorySizeFix(); // Common, dllmain
	WinCall_GetDiskFreeSpaceFix(); // Common, dllmain
	NoFrontMode();
}

uintptr_t mcoExecutableInit_Sunset_RetPtr = 0x52437E;
__declspec(naked) void MCOExecutableInit_Sunset_asmPart()
{
	_asm
	{
		call InitSunsetBase
		push 0x0 // orig
		push 0x0 // orig
		push 0x0 // orig
		jmp mcoExecutableInit_Sunset_RetPtr
	}
}

void InitSunset()
{
	injector::MakeNOP(0x524378, 6);
	injector::MakeJMP(0x524378, MCOExecutableInit_Sunset_asmPart);
}