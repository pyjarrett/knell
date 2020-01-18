/*
 * High-level render control.
 *
 * High-level control is the actual goals to accomplish during rendering, such
 * as indicating frame start/end, clearing the screen, drawing a background, or
 * drawing a sprite at a specific location.  The details of what exactly is
 * happening is hidden from the game itself since the high-level control hides
 * the details of how these details occur.  Rendering occurs only to a single
 * rectangular window.
 *
 * Any sort of rendering backend should be able to implement the functions
 * defined here and be able to render the scene appropriately.
 *
 * This engine supports 2D graphics only.  This includes sprites, lines,
 * text, and polygons.
 */
#ifndef KN_RENDER_HL_H
#define KN_RENDER_HL_H

#include "kn.h"

KN_API void R_Init(uint32_t width, uint32_t height);
KN_API void R_StartFrame();
KN_API void R_EndFrame();

#endif /* KN_RENDER_HL_H */
