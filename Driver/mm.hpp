#pragma once

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
#include <fltKernel.h>
#include <minwindef.h>
#include <windef.h>
#include <ntimage.h>
#ifdef __cplusplus
}
#endif // __cplusplus

#include "aob.hpp"

#define PML4I_SHIFT     39
#define PDPTI_SHIFT     30
#define PDI_SHIFT       21
#define PTI_SHIFT       12

#define GET_1G_PAGE_OFFSET(a) ((a) & 0x3FFFFFFF)
#define GET_2M_PAGE_OFFSET(a) ((a) & 0x1FFFFF)
#define GET_4K_PAGE_OFFSET(a) ((a) & 0xFFF)

#define GET_1G_PFN_PAGE(a) (a << 30)
#define GET_2M_PFN_PAGE(a) (a << 21)
#define GET_4K_PFN_PAGE(a) (a << 12)

#define PAGE_SIZE_4K 0x1000
#define PAGE_SIZE_2M 0x200000
#define PAGE_SIZE_1G 0x40000000
#define PAGE_SIZE_512G 0x8000000000

#define PAGE_ENTRY_NUM    512


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



NTSTATUS ReadHPA(
    _In_ UINT64 Addr,
    _Out_ PVOID Buffer,
    _In_ UINT64 Size,
    _Out_ UINT64& ReadBytes
);

BOOLEAN GPA2HPA(
    _In_ UINT64 GPA,
    _In_ UINT64 EPTP,
    _Out_ UINT64& HPA
);

NTSTATUS ReadGPA(
    _In_ UINT64 GPA,
    _In_ UINT64 EPTP,
    _Out_ PVOID Buffer,
    _In_ UINT64 BufferSize,
    _Out_ UINT64& ReadBytes
);

BOOLEAN GVA2GPA(
    _In_ UINT64 GVA,
    _In_ UINT64 CR3,
    _In_ UINT64 EPTP,
    _Out_ UINT64& GPA
);

NTSTATUS ReadGVA(
    _In_ UINT64 GVA,
    _In_ UINT64 CR3,
    _In_ UINT64 EPTP,
    _Out_ PVOID Buffer,
    _In_ UINT64 BufferSize,
    _Out_ UINT64& ReadBytes
);

NTSTATUS GetNtosBaseByEnumPageTable(
    _In_ UINT64 CR3,
    _In_ UINT64 EPTP,
    _Out_ UINT64& NtoskrnlGPA,
    _Out_ UINT64& NtoskrnlGVA
);






NTSTATUS WalkPageTableSelf(
    _In_ UINT64 CR3,
    _In_ UINT64 EPTP,
    _In_ PVOID Buffer,
    _In_ UINT64 BufferSize,
    _Out_ UINT64& ReadBytes
);
