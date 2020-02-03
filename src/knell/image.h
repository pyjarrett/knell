#ifndef KN_IMAGE_H
#define KN_IMAGE_H

#include <knell/kn.h>
#include <knell/memory.h>

/**
 * Images are bulk groups of pixel data stored in memory and must be fed into
 * the renderer to create a texture to use for actual drawing.
 */
typedef struct {
	DynamicBuffer pixels;

	/*
	 * This imposes a limit of ~4 million pixels per side.  There probably
	 * shouldn't be any levels even of that size.
	 */
	uint32_t width, height;
} ImagePixels;

bool Image_Allocate(ImagePixels* image, const char* fileName);
void Image_Free(ImagePixels* image);

#endif /* KN_IMAGE_H */
