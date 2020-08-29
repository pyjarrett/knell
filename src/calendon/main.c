#include "main.h"

#include <calendon/assets.h>
#include <calendon/assets-fileio.h>
#include <calendon/control.h>
#include <calendon/crash.h>
#include <calendon/log.h>
#include <calendon/main-config.h>
#include <calendon/memory.h>
#include <calendon/plugin.h>
#ifdef _WIN32
#include <calendon/process.h>
#endif
#include <calendon/path.h>
#include <calendon/string.h> // for config
#include <calendon/system.h>
#include <calendon/tick-limits.h>
#include <calendon/time.h>
#include <calendon/render.h>
#include <calendon/ui.h>

#include <errno.h>
#include <stdio.h>
#include <time.h>

static CnTime s_lastTick;
static CnPlugin s_payload;

enum { CnMaxNumCoreSystems = 16 };
static CnSystem s_coreSystems[CnMaxNumCoreSystems];
static uint32_t s_numCoreSystems = 0;

CnSystem cnMain_System(void);

void cnMain_AddCoreSystem(CnSystem system)
{
	CN_ASSERT(s_numCoreSystems < CnMaxNumCoreSystems, "Too many core systems added.");
	s_coreSystems[s_numCoreSystems] = system;
	++s_numCoreSystems;
};

bool cnMain_Init(void) {
	CnMainConfig* config = (CnMainConfig*)cnMain_Config();
	if (config->tickLimit != 0) {
		cnMain_SetTickLimit(config->tickLimit);
	}
	return true;
}

static void cnMain_BuildCoreSystemList(void)
{
	CnSystem_SystemFn systems[] = {
		cnMain_System,
		cnLog_System,
		cnCrash_System,
		cnMem_System,
		cnTime_System,
		cnAssets_System
	};

	for (uint32_t i = 0; i < CN_ARRAY_SIZE(systems); ++i) {
		cnMain_AddCoreSystem(systems[i]());
	}
}

void cnMain_ValidatePayload(CnPlugin* payload)
{
	CN_ASSERT(payload, "Cannot register a null payload.");

	if (!payload->init) CN_FATAL_ERROR("CnPlugin_Init function missing in payload.");
	if (!payload->draw) CN_FATAL_ERROR("CnPlugin_DrawFn function missing in payload.");
	if (!payload->tick) CN_FATAL_ERROR("CnPlugin_TickFn function missing in payload.");
	if (!payload->shutdown) CN_FATAL_ERROR("CnPlugin_ShutdownFn function missing in payload.");
}

void cnMain_LoadPayloadFromFile(const char* sharedLibraryName)
{
	CN_ASSERT(sharedLibraryName, "Cannot use a null shared library name to load a payload.");
	uint64_t gameLibModified;
	if (!cnAssets_LastModifiedTime(sharedLibraryName, &gameLibModified)) {
		CN_FATAL_ERROR("Unable to determine last modified time of '%s'", sharedLibraryName);
	}
	struct tm *lt = localtime((time_t*)&gameLibModified);
	char timeBuffer[80];
	strftime(timeBuffer, sizeof(timeBuffer), "%c", lt);
	CN_TRACE(LogSysMain, "Last modified time: %s", timeBuffer);

	// Shutdown any previous plugin.
	if (s_payload.shutdown) {
		s_payload.shutdown();
	}

	cnSharedLibrary_Release(s_payload.sharedLibrary);
	CnSharedLibrary library = cnSharedLibrary_Load(sharedLibraryName);
	if (!library) {
		CN_FATAL_ERROR("Unable to load game module: %s", sharedLibraryName);
	}
	cnPlugin_LoadFromSharedLibrary(&s_payload, library);

	cnMain_ValidatePayload(&s_payload);
}

void cnMain_PrintUsage(int argc, char** argv)
{
	// Print core systems.
	for (uint32_t i = 0; i < s_numCoreSystems; ++i) {
		CnSystem* system = &s_coreSystems[i];
		CnCommandLineOptionList optionList = system->options();
		if (system->name && optionList.numOptions > 0) {
			cnPrint("%s\n", system->name);
		}

		for (uint32_t optionIndex = 0; optionIndex < optionList.numOptions; ++optionIndex) {
			CnCommandLineOption* option = &optionList.options[optionIndex];
			const char* help = option->help ? option->help : "";
			cnPrint("%s\n", help);
		}
	}

	// Print the provided arguments.
	cnPrint("Arguments provided:\n");
	for (int i = 0; i < argc; ++i) {
		cnPrint("%4d: \"%s\"\n", i, argv[i]);
	}
}

/**
 * Attempt to parse the next command line option with a given system.
 */
int32_t cnMain_RunSystemParsers(CnCommandLineParse* commandLineParse, CnSystem* system)
{
	CN_ASSERT_PTR(commandLineParse);
	CN_ASSERT_PTR(system);

	CnCommandLineOptionList options = system->options();
	for (uint32_t parserIndex = 0; parserIndex < options.numOptions; ++parserIndex) {
		CnCommandLineOption *option = &options.options[parserIndex];
		if (cnCommandLineOption_Matches(option, commandLineParse)) {
			return option->parser(commandLineParse, system->config());
		}
	}
	return 0;
}

bool cnMain_ParseCommandLine(int argc, char** argv)
{
	CN_ASSERT(argc >= 1, "Argument count must at least include the executable.");
	CN_ASSERT_PTR(argv);

	for (uint32_t i = 0; i < s_numCoreSystems; ++i) {
		if (s_coreSystems[i].setDefaultConfig == NULL) {
			CN_ERROR(LogSysMain, "%s is missing a default config.", s_coreSystems[i].name);
		}
		CN_ASSERT_PTR(s_coreSystems[i].setDefaultConfig);
		s_coreSystems[i].setDefaultConfig(s_coreSystems[i].config());
	}

	CnCommandLineParse commandLineParse = cnCommandLineParse_Make(argc, argv);
	while (cnCommandLineParse_ShouldContinue(&commandLineParse)) {
		int argsParsed = 0;
		uint32_t systemIndex = 0;
		while (systemIndex < s_numCoreSystems && argsParsed == 0) {
			CnSystem* system = &s_coreSystems[systemIndex];
			argsParsed = cnMain_RunSystemParsers(&commandLineParse, system);
			if (argsParsed != 0) {
				break;
			}
			++systemIndex;
		}
		if (argsParsed <= 0) {
			cnPrint("Unable to parse argument: \"%s\" at index %d\n",
				cnCommandLineParse_LookAhead(&commandLineParse, 1),
				cnCommandLineParse_LookAheadIndex(&commandLineParse, 1));
			cnMain_PrintUsage(argc, argv);
			return false;
		}
		cnCommandLineParse_Advance(&commandLineParse, argsParsed);
	}
	return true;
}

/**
 * Common initialization point for all global systems.
 */
void cnMain_InitAllSystems(int argc, char** argv)
{
	cnMain_BuildCoreSystemList();
	cnLog_PreInit();

	if (!cnMain_ParseCommandLine(argc, argv)) {
		CN_FATAL_ERROR("Unable to parse command line.");
	}

	for (uint32_t i = 0; i < s_numCoreSystems; ++i) {
		if (!s_coreSystems[i].plugin().init()) {
			CN_FATAL_ERROR("Unable to initialize core system: %d", i);
		}
	}

#if CN_USE_CONFIG_FILES
	cnConfig_Init();
	{
		// Read every configuration file.
		CnConfigFile file = cnConfigFile_Load(fileName);

		// Apply each attribute in turn.
		CnConfigKey key = cnConfig_NextKey();
		CnConfigLine line = cnConfig_NextLine();

		const char* prefix = cnConfigKey_Prefix(key);
		CnSystemId id = cnConfig_PrefixToSystemId(prefix);

		const char8 value = cnConfigKey_Value(key);

		CnSystem* system;
		system->Configure();
	}
#endif

	// TODO: Resolution should be read from config or as a a configuration option.
	const uint32_t width = 1024;
	const uint32_t height = 768;

	CnUIInitParams uiInitParams;
	uiInitParams.resolution = (CnDimension2u32) { .width = width, .height = height };

	cnUI_Init(&uiInitParams);
	cnR_Init(uiInitParams.resolution);

	// If there is a demo to load from file, then use that.
	CnMainConfig* config = (CnMainConfig*)cnMain_Config();
	if (!cnPlugin_IsComplete(&config->payload)) {
		if (!cnPath_IsFile(config->gameLibPath.str)) {
			CN_FATAL_ERROR("Cannot load game. '%s' is not a game library.", config->gameLibPath.str);
		}

		const char* gameLib = config->gameLibPath.str;
		if (gameLib) {
			cnMain_LoadPayloadFromFile(gameLib);
		}
	}
	else {
		cnMain_ValidatePayload(&config->payload);
	}

	if (s_payload.init) {
		s_payload.init();
	}
	if (!s_payload.draw) CN_FATAL_ERROR("Draw function missing. Write a CnPlugin_Draw(void) function.");
	if (!s_payload.tick) CN_FATAL_ERROR("Update function missing. Write a CnPlugin_Tick(void) function.");

	s_lastTick = cnTime_MakeNow();

	CN_TRACE(LogSysMain, "Systems initialized.");

#ifdef _WIN32
	// TODO: This should be hidden unless doing an "diagnostic-startup-crash" or some other special behavior.
	//cnProc_PrintLoadedDLLs();
#endif
}

/**
 * Possibly generate a delta time for the next game update.  If the time since
 * the previous tick is too small or very large, no tick will be generated.
 * Small ticks do needless work, and large ticks might be due to resuming from
 * the debugger.
 *
 * @param[out] outDt delta time if a tick is generated (returns true), not set otherwise
 * @return true if a tick should occur
 */
bool cnMain_GenerateTick(CnTime* outDt)
{
	const CnTime current = cnTime_Max(s_lastTick, cnTime_MakeNow());

	// Prevent updating too rapidly.  Maintaining a relatively consistent
	// timestep limits stored state and prevents precision errors due to
	// extremely small dt.
	//
	// Since Calendon is single-threaded, VSync will probably ensure that the
	// minimum tick size is never missed.
	const CnTime minTickSize = cnTime_MakeMilli(8);
	const CnTime dt = cnTime_SubtractMonotonic(current, s_lastTick);
	if (cnTime_LessThan(dt, minTickSize)) {
		return false;
	}

	s_lastTick = current;

	// Ignore huge ticks, such as when resuming in the debugger.
	const CnTime maxTickSize = cnTime_MakeMilli(5000);
	if (cnTime_LessThan(maxTickSize, dt)) {
		CN_TRACE(LogSysMain, "Skipping large tick: %" PRIu64 " ms", cnTime_Milli(dt));
		return false;
	}

	*outDt = dt;
	return true;
}

void cnMain_StartUp(int argc, char** argv)
{
	cnMain_BuildCoreSystemList();
	cnLog_PreInit();

	if (!cnMain_ParseCommandLine(argc, argv)) {
		CN_FATAL_ERROR("Unable to parse command line.");
	}

	for (uint32_t i = 0; i < s_numCoreSystems; ++i) {
		if (!s_coreSystems[i].plugin().init()) {
			CN_FATAL_ERROR("Unable to initialize core system: %d", i);
		}
	}

#if CN_USE_CONFIG_FILES
	cnConfig_Init();
		{
			// Read every configuration file.
			CnConfigFile file = cnConfigFile_Load(fileName);

			// Apply each attribute in turn.
			CnConfigKey key = cnConfig_NextKey();
			CnConfigLine line = cnConfig_NextLine();

			const char* prefix = cnConfigKey_Prefix(key);
			CnSystemId id = cnConfig_PrefixToSystemId(prefix);

			const char8 value = cnConfigKey_Value(key);

			CnSystem* system;
			system->Configure();
		}
#endif

	// TODO: Resolution should be read from config or as a a configuration option.
	const uint32_t width = 1024;
	const uint32_t height = 768;

	CnUIInitParams uiInitParams;
	uiInitParams.resolution = (CnDimension2u32) { .width = width, .height = height };

	cnUI_Init(&uiInitParams);
	cnR_Init(uiInitParams.resolution);

	// If there is a demo to load from file, then use that.
	CnMainConfig* config = (CnMainConfig*) cnMain_Config();
	if (!cnPlugin_IsComplete(&config->payload)) {
		if (!cnPath_IsFile(config->gameLibPath.str)) {
			CN_FATAL_ERROR("Cannot load game. '%s' is not a game library.", config->gameLibPath.str);
		}

		const char* gameLib = config->gameLibPath.str;
		if (gameLib) {
			cnMain_LoadPayloadFromFile(gameLib);
		}
	}
	else {
		cnMain_ValidatePayload(&config->payload);
	}

	if (s_payload.init) {
		s_payload.init();
	}
	if (!s_payload.draw) CN_FATAL_ERROR("Draw function missing. Write a CnPlugin_Draw(void) function.");
	if (!s_payload.tick) CN_FATAL_ERROR("Update function missing. Write a CnPlugin_Tick(void) function.");

	s_lastTick = cnTime_MakeNow();

	CN_TRACE(LogSysMain, "Systems initialized.");

#ifdef _WIN32
// TODO: This should be hidden unless doing an "diagnostic-startup-crash" or some other special behavior.
//cnProc_PrintLoadedDLLs();
#endif
}

/**
 * The big loop which processes events, ticks and draws until the game is ready
 * to shut down.
 */
void cnMain_Loop(void)
{
	CN_ASSERT(s_payload.tick, "Tick function not defined.");
	CN_ASSERT(s_payload.draw, "Draw function not defined.");

	while (cnMain_IsRunning() && !cnMain_IsTickLimitReached())
	{
		// Event checking should be quick.  Always processing events prevents
		// slowness due to bursts.
		cnUI_ProcessWindowEvents();

		CnTime dt;
		if (cnMain_GenerateTick(&dt)) {
			s_payload.tick(dt);
			cnMain_TickCompleted();
		}
		s_payload.draw();
	}
}

void cnMain_Shutdown(void)
{
	if (s_payload.shutdown) s_payload.shutdown();

	cnR_Shutdown();
	cnUI_Shutdown();

	for (uint32_t i = 0; i < s_numCoreSystems; ++i) {
		const uint32_t nextSystemIndex = s_numCoreSystems - i - 1;

		CnSystem* system = &s_coreSystems[nextSystemIndex];
		if (!system->plugin().shutdown) {
			printf("No shutdown function for: %s\n", system->name);
		}
		else {
			system->plugin().shutdown();
		}
	}
}

CnPlugin cnMain_Plugin(void) {
	return (CnPlugin) {
		.init          = cnMain_Init,
		.shutdown      = NULL,
		.tick          = NULL,
		.draw          = NULL,
		.sharedLibrary = NULL
	};
}

CnSystem cnMain_System(void)
{
	return (CnSystem) {
		.name             = "Main",
		.options          = cnMain_CommandLineOptionList,
		.config           = cnMain_Config,
		.setDefaultConfig = cnMain_SetDefaultConfig,
		.plugin           = cnMain_Plugin
	};
}
