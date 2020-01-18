#ifndef KN_PROCESS_H
#define KN_PROCESS_H

#include <stdbool.h>

#ifdef _WIN32

// https://docs.microsoft.com/en-us/windows/win32/psapi/enumerating-all-modules-for-a-process?redirectedfrom=MSDN
/**
 * Prints all the loaded DLLs to the log.
 */
bool Proc_PrintLoadedDLLs(void);

#endif /* _WIN32 */

#endif /* KN_PROCESS_H */