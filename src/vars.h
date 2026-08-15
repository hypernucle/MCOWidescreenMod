#pragma once
#include <numeric>

inline bool dBuild = false;
inline bool TestConsole = false;
inline std::string gameFolderPath = "";

inline uint32_t screenWidth = 0;
inline uint32_t screenHeight = 0;
inline uint32_t sysScreenWidth = 0;
inline uint32_t sysScreenHeight = 0;
inline uint32_t aspectWidth = 0;
inline uint32_t aspectHeight = 0;
inline float_t aspectRatio = 0.0f;
inline uint32_t vmAspectWidth = 0;
inline uint32_t vmAspectHeight = 0;
inline uint32_t userAspectWidth = 0;
inline uint32_t userAspectHeight = 0;
//
inline float_t renderViewportHeightScale = 0.0f;
inline float_t renderViewportHeightWidescreenScale = 0.0f;
inline float_t renderInGameDefaultFOV = 0.0f;
inline float_t renderInGameDefaultWidescreenFOV = 0.0f;
inline float_t renderInGameInteriorFOV = 0.0f;
inline float_t horFactor = 0.0f;
//
inline uint32_t uiTargetScreenWidth = 0;
inline float_t uiTarget2SizeWidthResizer = 0.0f;
inline float_t uiMapSizeMultiplier = 0.0f;
inline uint32_t uiScreenSizeWidth = 0;
inline float_t uiTargetMarginX = 0.0f;
inline uint32_t uiTargetMarginX_Int = 0;
//
inline float_t uiLoadingScreen_TipsX = 0.0f;
inline float_t uiLoadingScreen_ProgressBarWidth = 0.0f;
inline float_t uiCountdown_TrafficLightStyle_X = 0.0f;
inline float_t uiCountdown_TrafficLight2Style_X = 0.0f;
inline float_t uiCountdown_DragModeStyle_X = 0.0f;
//
inline uint32_t movieViewScaleMultiplier = 2;