#ifndef CN_MEMORY_H
#define CN_MEMORY_H

#include <calendon/cn.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A single contiguous block of dynamically allocated memory.
 *
 * Allocated with `cnMem_Allocate` and then released with `cnMem_Free`.  The number
 * of unfree'd buffers is reported on shutdown.
 */
typedef struct {
	char* contents;
	uint32_t size;
} CnDynamicBuffer;

CN_API void cnMem_Init(void);
CN_API void cnMem_Shutdown(void);
CN_API void cnMem_Allocate(CnDynamicBuffer* buffer, uint32_t size);
CN_API void cnMem_Free(CnDynamicBuffer* buffer);

#ifdef __cplusplus
}
#endif

#endif /* CN_MEMORY_H */