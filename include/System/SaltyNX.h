#pragma once
#include <switch.h>

extern Handle saltysd_orig;

#ifdef __cplusplus
extern "C" {
#endif

Result SaltySD_Connect();
Result SaltySD_Term();
Result SaltySD_CheckIfSharedMemoryAvailable(ptrdiff_t *offset, u64 size);
Result SaltySD_GetSharedMemoryHandle(Handle *retrieve);
Result SaltySD_GetDisplayRefreshRate(uint8_t* refreshRate);
Result SaltySD_SetDisplayRefreshRate(uint8_t refreshRate);

#ifdef __cplusplus
}
#endif