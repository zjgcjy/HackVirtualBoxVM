#pragma once

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <fltKernel.h>

#ifdef __cplusplus
}
#endif // __cplusplus


#define PML4I_SHIFT     39
#define PDPTI_SHIFT     30
#define PDI_SHIFT       21
#define PTI_SHIFT       12

#define GET_1G_IN_PAGE_OFFSET(a) ((a) & 0x3FFFFFFF)
#define GET_2M_IN_PAGE_OFFSET(a) ((a) & 0x1FFFFF)
#define GET_4K_IN_PAGE_OFFSET(a) ((a) & 0xFFF)


typedef union _PML4E {
    struct {
        UINT64 Present : 1;
        UINT64 Write : 1;
        UINT64 User : 1;
        UINT64 PageLevelWriteThrough : 1;
        UINT64 PageLevelCacheDisable : 1;
        UINT64 Accessed : 1;
        UINT64 Reserved1 : 1;
        UINT64 MustBeZero : 1;
        UINT64 Ignored1 : 4;
        UINT64 PageFrameNumber : 36;
        UINT64 Ignored2 : 11;
        UINT64 ExecuteDisable : 1;
    } s;
    UINT64 AsUInt;
} PML4E, * PPML4E;

typedef union _PDPTE_LARGE {
    struct {
        UINT64 Present : 1;
        UINT64 Write : 1;
        UINT64 User : 1;
        UINT64 PageLevelWriteThrough : 1;
        UINT64 PageLevelCacheDisable : 1;
        UINT64 Accessed : 1;
        UINT64 Dirty : 1;
        UINT64 LargePage : 1;
        UINT64 Global : 1;
        UINT64 Ignored1 : 3;
        UINT64 Pat : 1;
        UINT64 Reserved1 : 17;
        UINT64 PageFrameNumber : 18;
        UINT64 Reserved2 : 4;
        UINT64 Ignored2 : 7;
        UINT64 ProtectionKey : 4;
        UINT64 ExecuteDisable : 1;
    } s;
    UINT64 AsUInt;
} PDPTE_LARGE, * PPDPTE_LARGE;

typedef union _PDPTE {
    struct {
        UINT64 Present : 1;
        UINT64 Write : 1;
        UINT64 User : 1;
        UINT64 PageLevelWriteThrough : 1;
        UINT64 PageLevelCacheDisable : 1;
        UINT64 Accessed : 1;
        UINT64 Reserved1 : 1;
        UINT64 LargePage : 1;
        UINT64 Ignored1 : 4;
        UINT64 PageFrameNumber : 36;
        UINT64 Reserved2 : 4;
        UINT64 Ignored2 : 11;
        UINT64 ExecuteDisable : 1;
    } s;
    UINT64 AsUInt;
} PDPTE, * PPDPTE;

typedef union _PDE_LARGE {
    struct {
        UINT64 Present : 1;
        UINT64 Write : 1;
        UINT64 User : 1;
        UINT64 PageLevelWriteThrough : 1;
        UINT64 PageLevelCacheDisable : 1;
        UINT64 Accessed : 1;
        UINT64 Dirty : 1;
        UINT64 LargePage : 1;
        UINT64 Global : 1;
        UINT64 Ignored1 : 3;
        UINT64 Pat : 1;
        UINT64 Reserved1 : 8;
        UINT64 PageFrameNumber : 27;
        UINT64 Reserved2 : 4;
        UINT64 Ignored2 : 7;
        UINT64 ProtectionKey : 4;
        UINT64 ExecuteDisable : 1;
    } s;
    UINT64 AsUInt;
} PDE_LARGE, * PPDE_LARGE;

typedef union _PDE {
    struct {
        UINT64 Present : 1;
        UINT64 Write : 1;
        UINT64 User : 1;
        UINT64 PageLevelWriteThrough : 1;
        UINT64 PageLevelCacheDisable : 1;
        UINT64 Accessed : 1;
        UINT64 Reserved1 : 1;
        UINT64 LargePage : 1;
        UINT64 Ignored1 : 4;
        UINT64 PageFrameNumber : 36;
        UINT64 Reserved2 : 4;
        UINT64 Ignored2 : 11;
        UINT64 ExecuteDisable : 1;
    } s;
    UINT64 AsUInt;
} PDE, * PPDE;

typedef union _PTE {
    struct {
        UINT64 Present : 1;
        UINT64 Write : 1;
        UINT64 User : 1;
        UINT64 PageLevelWriteThrough : 1;
        UINT64 PageLevelCacheDisable : 1;
        UINT64 Accessed : 1;
        UINT64 Dirty : 1;
        UINT64 Pat : 1;
        UINT64 Global : 1;
        UINT64 Ignored1 : 3;
        UINT64 PageFrameNumber : 36;
        UINT64 Reserved1 : 4;
        UINT64 Ignored2 : 7;
        UINT64 ProtectionKey : 4;
        UINT64 ExecuteDisable : 1;
    } s;
    UINT64 AsUInt;
} PTE, * PPTE;


