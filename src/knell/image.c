#include <knell/image.h>

#include <knell/kn.h>
#include <knell/assets-fileio.h>
#include <knell/compat-spng.h>
#include <knell/log.h>

extern LogHandle LogSysAssets;

void Image_Flip(ImagePixels* image)
{
	KN_ASSERT(image != NULL, "Cannot load flip a null image.");
	KN_ASSERT(image->pixels.size > 0, "No pixels to flip.");
	KN_ASSERT(image->width > 0, "Cannot flip an image with no width.");
	KN_ASSERT(image->height > 0, "Cannot flip an image with no height.");

	DynamicBuffer flippedImage;
	Mem_Allocate(&flippedImage, image->pixels.size);

	// Assume RGBA8 encoding.
	const uint32_t pixelSize = 4 * sizeof(uint8_t);

	const uint32_t expectedPixelStorageSize = pixelSize * image->width * image->height;
	KN_ASSERT(expectedPixelStorageSize == image->pixels.size,
		"Excessive storage for pixels found %" PRIu32 ", not matching resolution "
		"%" PRIu32 "(%" PRIu32 ", %" PRIu32 ")", expectedPixelStorageSize,
		image->pixels.size, image->width, image->height);

	const uint32_t rowSize = pixelSize * image->width;

	uint8_t* from = NULL;
	uint8_t* to = NULL;

	// Flip one row at a time.
	for (uint32_t i = 0; i < image->height; ++i) {
		from = (uint8_t*)flippedImage.contents + rowSize * (image->height - i - 1);
		to = (uint8_t*)image->pixels.contents + rowSize * i;
		memcpy(from, to, rowSize);
	}

	// Swap the buffer content pointers.
	char* temp = image->pixels.contents;
	image->pixels.contents = flippedImage.contents;
	flippedImage.contents = temp;

	Mem_Free(&flippedImage);
}

/**
 * Using `Allocate` as the name here to ensure the clients know to call
 * `Image_Free`, and don't need to manually free the stored buffer of pixels.
 *
 * @todo support image types other than RGBA8.
 */
bool Image_Allocate(ImagePixels* image, const char* fileName)
{
	KN_ASSERT(image != NULL, "Cannot load data into a null image.");
	KN_ASSERT(fileName != NULL, "Cannot load an image with a null file name.");
	DynamicBuffer fileBuffer;

	if (!Assets_ReadFile(fileName, KN_FILE_TYPE_BINARY, &fileBuffer)) {
		KN_WARN(LogSysAssets, "Unable to load image from %s", fileName);
		return false;
	}

	spng_ctx* pngContext = spng_ctx_new(0);
	spng_set_png_buffer(pngContext, fileBuffer.contents, fileBuffer.size);

	uint32_t format = SPNG_FMT_RGBA8;
	size_t decodedSize = 0;
	spng_decoded_image_size(pngContext, format, &decodedSize);

	Mem_Allocate(&image->pixels, (uint32_t)decodedSize);
	image->pixels.size = (uint32_t)decodedSize;

	spng_decode_image(pngContext, (uint8_t*)image->pixels.contents, image->pixels.size, format, 0);

	struct spng_ihdr header;
	spng_get_ihdr(pngContext, &header);
	spng_ctx_free(pngContext);

	image->width = header.width;
	image->height = header.height;

	Image_Flip(image);

	KN_TRACE(LogSysAssets, "Loading image: %s", fileName);
	KN_TRACE(LogSysAssets, "Image size %d, %d", header.width, header.height);
	KN_TRACE(LogSysAssets, "Output size: %llu", decodedSize);
	KN_TRACE(LogSysAssets, "Input fileContents size: %d", fileBuffer.size);

    Mem_Free(&fileBuffer);

	return true;
}

void Image_Free(ImagePixels* image)
{
	KN_ASSERT(image != NULL, "Cannot load data into a null image.");
	Mem_Free(&image->pixels);
}