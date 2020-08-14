#include "main-config.h"

#include <calendon/cn.h>
#include <calendon/argparse.h>
#include <calendon/env.h>
#include <calendon/string.h>

#include <errno.h>
#include <string.h>

int32_t cnMain_OptionPayload(const CnCommandLineParse* parse, void* c);
int32_t cnMain_OptionTickLimit(const CnCommandLineParse* parse, void* c);

static CnMainConfig s_config;
static CnCommandLineOption s_options[] = {
	{
		"-g,--game SHARED_LIB       Change the game/demo to boot.\n",
		"-g",
		"--game",
		cnMain_OptionPayload
	},
	{
		"-t,--tick-limit NUM_TICKS  Limit the run to a specific number of ticks.\n",
		"-t",
		"--tick-limit",
		cnMain_OptionTickLimit
	}
};

CnCommandLineOptionList cnMain_CommandLineOptionList(void)
{
	return (CnCommandLineOptionList) {
		.options = s_options,
		.numOptions = 2
	};
}

void* cnMain_Config(void)
{
	return &s_config;
}

void cnMain_SetDefaultConfig(void* config)
{
	CnMainConfig* c = (CnMainConfig*)config;
	memset(c, 0, sizeof(CnMainConfig));
}

int32_t cnMain_OptionPayload(const CnCommandLineParse* parse, void* c)
{
	CN_ASSERT_NOT_NULL(parse);
	CN_ASSERT_NOT_NULL(c);

	CnMainConfig* config = (CnMainConfig*)c;

	if (!cnCommandLineParse_HasLookAhead(parse, 2)) {
		cnPrint("Payload must be provided a shared library (or DLL) to load\n");
		return CnOptionParseError;
	}

	const char* gamePath = cnCommandLineParse_LookAhead(parse, 2);
	if (cnString_TerminatedFitsIn(gamePath, CN_MAX_TERMINATED_PATH)) {
		if (!cnPath_IsFile(gamePath)) {
			char cwd[CN_MAX_TERMINATED_PATH];
			cnEnv_CurrentWorkingDirectory(cwd, CN_MAX_TERMINATED_PATH);
			cnPrint("Current working directory is: %s\n", cwd);
			cnPrint("Game library %s does not exist.\n", gamePath);
			return CnOptionParseError;
		}
		cnPathBuffer_Set(&config->gameLibPath, gamePath);
		cnPrint("Game library: '%s'\n", config->gameLibPath.str);
		return 2;
	}
	else {
		cnPrint( "Length of name of game library is too long.");
		return CnOptionParseError;
	}
}

int32_t cnMain_OptionTickLimit(const CnCommandLineParse* parse, void* c)
{
	CN_ASSERT_NOT_NULL(parse);
	CN_ASSERT_NOT_NULL(c);

	CnMainConfig* config = (CnMainConfig*)c;

	if (!cnCommandLineParse_HasLookAhead(parse, 2)) {
		cnPrint("Must provide the number of ticks for which to run the program.\n");
		return CnOptionParseError;
	}

	const char* tickString = cnCommandLineParse_LookAhead(parse, 2);
	char* readCursor;
	const int64_t parsedValue = strtoll(tickString, &readCursor, 10);
	if (parsedValue < 0) {
		cnPrint("Cannot step a negative number of ticks: %s\n", tickString);
		return CnOptionParseError;
	}

	if (*readCursor != '\0' || errno == ERANGE) {
		cnPrint("Unable to parse tick limit: %s\n", tickString);
		return CnOptionParseError;
	}
	config->tickLimit = parsedValue;
	return 2;
}

void cnMainConfig_Freestanding(CnPlugin_InitFn init, CnPlugin_TickFn tick,
	CnPlugin_DrawFn draw, CnPlugin_ShutdownFn shutdown)
{
	s_config.payload.init = init;
	s_config.payload.tick = tick;
	s_config.payload.draw = draw;
	s_config.payload.shutdown = shutdown;
	s_config.payload.sharedLibrary = NULL;
}
