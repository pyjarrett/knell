/*
 * Working with animation loops.
 */
#include <calendon/cn.h>
#include <calendon/anim-loop.h>
#include <calendon/assets.h>
#include <calendon/log.h>
#include <calendon/math2.h>
#include <calendon/path.h>
#include <calendon/render.h>
#include <calendon/render-resources.h>
#include <calendon/time.h>

#include <math.h>

CnLogHandle LogSysSample;

CnAnimationLoopCursor sampleCursor;
CnAnimationLoop sampleLoop;

CnTransform2 rotate;

#define SPRITE_ANIMATION_FRAMES 3
CnSpriteId spriteFrames[SPRITE_ANIMATION_FRAMES];

#define NUM_CIRCLE_VERTICES 20
CnFloat2 circleOrigin;
CnFloat2 circleVertices[NUM_CIRCLE_VERTICES];

CnFontId font;
static CnTime lastDt;

/**
 * Creates a line of points to form circle in a counter clockwise winding.
 */
void cnRLL_CreateCircle(CnFloat2* vertices, uint32_t numVertices, float radius)
{
	CN_ASSERT(vertices != NULL, "Cannot write vertices into a null pointer");
	CN_ASSERT(radius > 0.0f, "Radius must positive: %f provided", radius);
	const float arcSize = 2 * 3.14159f / (numVertices - 1);
	for (uint32_t i = 0; i < numVertices - 1; ++i) {
		vertices[i] = cnFloat2_Make(radius * cosf(i * arcSize),
									radius * sinf(i * arcSize));
	}
	vertices[numVertices - 1] = vertices[0];
}

CN_GAME_API bool CnPlugin_Init(void)
{
	cnLog_RegisterSystem(&LogSysSample, "Sample", CnLogVerbosityTrace);
	CN_TRACE(LogSysSample, "Sample loaded");

	CN_TRACE(LogSysSample, "Animation loop size:        %zu bytes", sizeof(CnAnimationLoop));
	CN_TRACE(LogSysSample, "Animation loop cursor size: %zu bytes", sizeof(CnAnimationLoopCursor));

	sampleLoop.numStates = SPRITE_ANIMATION_FRAMES;
	sampleLoop.elapsed[0] = cnTime_MakeMilli(150);
	sampleLoop.elapsed[1] = cnTime_MakeMilli(150);
	sampleLoop.elapsed[2] = cnTime_MakeMilli(150);

	cnR_CreateSprite(&spriteFrames[0]);
	cnR_CreateSprite(&spriteFrames[1]);
	cnR_CreateSprite(&spriteFrames[2]);

	const char* frameFilenames[] = {
		"sprites/stick_person.png",
		"sprites/stick_person2.png",
		"sprites/stick_person3.png"
	};

	for (uint32_t i = 0; i < 3; ++i) {
		CnPathBuffer path;
		cnAssets_PathBufferFor(frameFilenames[i], &path);
		cnR_LoadSprite(spriteFrames[i], path.str);
	}

	rotate = cnTransform2_MakeIdentity();

	circleOrigin = cnFloat2_Make(400.0f, 400.0f);
	cnRLL_CreateCircle(circleVertices, NUM_CIRCLE_VERTICES, 50.0f);
	for (uint32_t i = 0; i < NUM_CIRCLE_VERTICES; ++i) {
		circleVertices[i] = cnFloat2_Add(circleVertices[i], circleOrigin);
	}

	CnPathBuffer fontPath;
	cnAssets_PathBufferFor("fonts/bizcat.psf", &fontPath);
	cnR_CreateFont(&font);
	if (!cnR_LoadPSF2Font(font, fontPath.str))	{
		CN_FATAL_ERROR("Unable to load font: %s", fontPath.str);
	}
	return true;
}

CN_GAME_API void CnPlugin_Draw(void)
{
	cnR_StartFrame();

	cnR_DrawDebugFullScreenRect();

	CnFloat2 position = cnFloat2_Make(100, 100);
	CnDimension2f size = { .width = 100.0f, .height = 100.0f };
	cnR_DrawSprite(spriteFrames[sampleCursor.current], position, size);

	CnRGB8u red = { .r = 255, .g = 0, .b = 0 };
	CnRGB8u green = { .r = 0, .g = 255, .b = 0 };
	CnRGB8u blue = { .r = 0, .g = 0, .b = 255 };

	cnR_DrawDebugLineStrip(circleVertices, NUM_CIRCLE_VERTICES, red);
	static int step = 0;
	--step;
	if (step < 0) step = NUM_CIRCLE_VERTICES - 2;
	cnR_DrawDebugLine(circleOrigin.x, circleOrigin.y, circleVertices[step].x,
					  circleVertices[step].y, green);

	CnTransform2 smallRotate = cnTransform2_MakeRotation(cnPlanarAngle_MakeDegrees(1));
	rotate = cnTransform2_Combine(rotate, smallRotate);
	const CnTransform2 transform = cnTransform2_Combine(cnTransform2_MakeTranslateXY(800, 600), rotate);

	cnR_DrawRect(cnFloat2_Make(0, 0), (CnDimension2f) { .width = 200.0f, .height = 300.0f}, red, transform);

	CnFloat2 rectPosition = cnFloat2_Make(200, 100);
	CnDimension2f rectSize = { .width = 100.0f, .height = 100.0f };
	cnR_DrawDebugRect(rectPosition, rectSize, green);

	cnR_DrawSimpleText(font, cnFloat2_Make(300, 100), "Hello, Paul!\xe2\x86\x93→\xe2\x86\x92");
	cnR_DrawSimpleText(font, cnFloat2_Make(100, 500), "«café, caffè» ™ © Â ←");

	static char frameTime[100] = "";
	lastDt = cnTime_Max(cnTime_MakeMilli(1), lastDt);
	static int fpsTick = 0;
	if (++fpsTick % 10 == 0) {
		fpsTick = 0;
		sprintf(frameTime, "FPS: %.1f", 1000.0f / cnTime_Milli(lastDt));
	}

	cnR_DrawSimpleText(font, cnFloat2_Make(0, 600), frameTime);
	cnR_EndFrame();
}

CN_GAME_API void CnPlugin_Tick(CnTime dt)
{
	cnAnimLoop_Tick(&sampleLoop, &sampleCursor, dt);
	lastDt = dt;
}

CN_GAME_API void CnPlugin_Shutdown(void)
{
}
