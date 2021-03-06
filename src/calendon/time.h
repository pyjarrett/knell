#ifndef CN_TIME_H
#define CN_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <calendon/cn.h>

#include <calendon/system.h>

/**
 * @file time.h
 *
 * An abstract representation of time, which can be extracted as a known period
 * or duration in a specific set of units.
 *
 * Many problems appear when dealing with time as a raw integer without typing
 * information:
 * - underflow due to subtraction
 * - using an integer in one unit type as another unit type
 * - difficulties understanding the units of a time integer being passed
 */

CN_API CnSystem cnTime_System(void);

/*
 * Functions for getting the current time and converting other units to the
 * same units being used for time.
 */
CN_API uint64_t cnTime_MsToNs(uint64_t ms);
CN_API uint64_t cnTime_NsToMs(uint64_t ns);
CN_API uint64_t cnTime_SecToNs(uint64_t sec);

CN_API CnTime   cnTime_MakeNow(void);
CN_API CnTime   cnTime_MakeZero(void);
CN_API CnTime   cnTime_MakeMilli(uint64_t millis);

CN_API uint64_t cnTime_Milli(CnTime t);
CN_API bool     cnTime_IsZero(CnTime t);

CN_API CnTime   cnTime_Add(CnTime left, CnTime right);
CN_API CnTime   cnTime_SubtractMonotonic(CnTime left, CnTime right);

CN_API bool     cnTime_LessThan(CnTime left, CnTime right);

CN_API float    cnTime_Lerp(CnTime currentDuration, CnTime totalDuration);

CN_API CnTime   cnTime_Min(CnTime left, CnTime right);
CN_API CnTime   cnTime_Max(CnTime left, CnTime right);

CN_API uint64_t cnUInt64_SubtractMonotonic(uint64_t left, uint64_t right);

#ifdef __cplusplus
}
#endif

#endif /* CN_TIME_H */
