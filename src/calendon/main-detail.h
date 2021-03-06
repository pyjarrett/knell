#ifndef CN_MAIN_DETAIL_H
#define CN_MAIN_DETAIL_H

#include <calendon/cn.h>

#include <calendon/log.h>
#include <calendon/main-config.h>
#include <calendon/behavior.h>
#include <calendon/system.h>
#include <calendon/time.h>

#ifdef __cplusplus
extern "C" {
#endif

extern CnTime s_lastTick;
extern CnBehavior s_payload;

enum { CnMaxNumCoreSystems = 16 };
extern CnSystem s_coreSystems[CnMaxNumCoreSystems];
extern uint32_t s_numCoreSystems;
void cnMain_InitCoreSystems(void);
void cnMain_BuildCoreSystemList(void);

bool cnMain_ParseCommandLine(int argc, char** argv);

void cnMain_StartUpUI(void);
void cnMain_LoadPayload(CnMainConfig* config);
void cnMain_ValidatePayload(CnBehavior* payload);
bool cnMain_GenerateTick(CnTime* outDt);

#ifdef __cplusplus
}
#endif

#endif /* CN_MAIN_DETAIL_H */