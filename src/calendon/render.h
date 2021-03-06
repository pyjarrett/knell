#ifndef CN_RENDER_H
#define CN_RENDER_H

/**
 *
 * @file render.h
 *
 * High-level render control.
 *
 * High-level control is the actual goals to accomplish during rendering, such
 * as indicating frame start/end, clearing the screen, drawing a background, or
 * drawing a sprite at a specific location.  The details of what exactly is
 * happening is hidden from the game itself since the high-level control hides
 * the details of how these details occur.  Rendering occurs only to a single
 * rectangular window.
 *
 * This engine supports 2D graphics only.  This includes sprites, lines,
 * text, and polygons.
 *
 * ## Design Notes
 *
 * There's a lot of duplication between this and the `render-ll.h`.  The
 * intention is to allow for cases where the interface provided to game clients
 * requires conversion to appropriate 3D functions to operate the back-end.
 */

#include <calendon/cn.h>

#include <calendon/color.h>
#include <calendon/math2.h>
#include <calendon/render-resources.h>

#ifdef __cplusplus
extern "C" {
#endif

CN_API void cnR_Init(CnDimension2u32 resolution);
CN_API void cnR_Shutdown(void);

CN_API void cnR_StartFrame(void);
CN_API void cnR_EndFrame(void);

CN_API CnDimension2u32 cnR_Resolution(void);

CN_API CnAABB2 cnR_BackingCanvasAABB2(void);

CN_API CnAABB2 cnR_Viewport(void);
CN_API void cnR_SetViewport(CnAABB2 viewport);

CN_API CnAABB2 cnR_CameraAABB2(void);
CN_API void cnR_SetCameraAABB2(CnAABB2 area);

CN_API bool cnR_CreateSprite(CnSpriteId* id);
CN_API bool cnR_LoadSprite(CnSpriteId id, const char* path);
CN_API void cnR_DrawSprite(CnSpriteId id, CnFloat2 position, CnDimension2f size);

CN_API bool cnR_CreateFont(CnFontId* id);
CN_API bool cnR_LoadPSF2Font(CnFontId id, const char* path);
CN_API void cnR_DrawSimpleText(CnFontId id, CnFloat2 position, const char* text);

CN_API void cnR_DrawDebugFullScreenRect(void);
CN_API void cnR_DrawDebugRect(CnFloat2 center, CnDimension2f dimensions, CnOpaqueColor color);
CN_API void cnR_DrawDebugLine(float x1, float y1, float x2, float y2, CnOpaqueColor color);
CN_API void cnR_DrawDebugLineStrip(CnFloat2* points, uint32_t numPoints, CnOpaqueColor color);
CN_API void cnR_DrawDebugFont(CnFontId id, CnFloat2 center, CnDimension2f size);

CN_API void cnR_DrawRect(CnFloat2 center, CnDimension2f dimensions, CnOpaqueColor color, CnTransform2 transform);
CN_API void cnR_OutlineRect(CnFloat2 center, CnDimension2f dimensions, CnOpaqueColor color, CnTransform2 transform);
CN_API void cnR_OutlineCircle(CnFloat2 center, float radius, CnOpaqueColor color, uint32_t numSegments);

CN_API void cnR_FillScreen(CnOpaqueColor color);

#ifdef __cplusplus
}
#endif

#endif /* CN_RENDER_H */
