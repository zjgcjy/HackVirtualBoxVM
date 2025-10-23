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

#define GET_1G_PAGE_OFFSET(a) ((a) & 0x3FFFFFFF)
#define GET_2M_PAGE_OFFSET(a) ((a) & 0x1FFFFF)
#define GET_4K_PAGE_OFFSET(a) ((a) & 0xFFF)

#define GET_1G_PFN_PAGE(a) (a << 30)
#define GET_2M_PFN_PAGE(a) (a << 21)
#define GET_4K_PFN_PAGE(a) (a << 12)


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

typedef NTSTATUS(*fpMmCopyMemory)(PVOID TargetAddress, MM_COPY_ADDRESS SourceAddress, SIZE_T NumberOfBytes, ULONG Flags, PSIZE_T NumberOfBytesTransferred);

fpMmCopyMemory g_MmCopyMemory = NULL;

//const ULONG g_KernelSpaceBase = 0xffff000000000000;

NTSTATUS ReadPhysicalMemory(
    _In_ UINT64 Addr,
    _Out_ PVOID Buffer,
    _In_ UINT64 Size,
    _Out_ UINT64& BytesTransferred
)
{
    if (g_MmCopyMemory == NULL)
    {
        UNICODE_STRING FunctionName;
        RtlInitUnicodeString(&FunctionName, L"MmCopyMemory");
        g_MmCopyMemory = (fpMmCopyMemory)MmGetSystemRoutineAddress(&FunctionName);
    }
    if (!Buffer)
    {
        return STATUS_INVALID_PARAMETER;
    }

    MM_COPY_ADDRESS SrcAddr = {0};
    SrcAddr.PhysicalAddress.QuadPart = Addr;

    return MmCopyMemory(Buffer, SrcAddr, Size, MM_COPY_MEMORY_PHYSICAL, &BytesTransferred);
}


NTSTATUS ParsePageTableSelf(
    _In_ UINT64 VA,
    _In_ UINT64 CR3,
    _Out_ PVOID Buffer,
    _In_ UINT64 BufferSize,
    _Out_ UINT64& ReadBytes)
{
    if (!Buffer)
    {
        return STATUS_INVALID_PARAMETER;
    }

    UINT64 Pml4i = (VA >> PML4I_SHIFT) & 0xFFF;
    UINT64 Pdpti = (VA >> PDPTI_SHIFT) & 0xFFF;
    UINT64 Pdi = (VA >> PDI_SHIFT) & 0xFFF;
    UINT64 Pti = (VA >> PTI_SHIFT) & 0xFFF;

    PML4E Pml4e = { 0 };
    PDPTE Pdpte = { 0 };
    PDE Pde = { 0 };
    PTE Pte = { 0 };

    UINT64 RetBytes = 0;
    CR3 &= ~0xFFF;
    NTSTATUS Status = ReadPhysicalMemory(CR3 + 8 * Pml4i, &Pml4e, sizeof(Pml4e), RetBytes);
    if (!NT_SUCCESS(Status) || !Pml4e.s.Present)
    {
        MyDbgPrint("Pml4e invalid");
        return Status;
    }
    MyDbgPrint("Pml4e pfn=%llx", Pml4e.s.PageFrameNumber);

    Status = ReadPhysicalMemory(GET_4K_PFN_PAGE(Pml4e.s.PageFrameNumber) + 8 * Pdpti, &Pdpte, sizeof(Pdpte), RetBytes);
    if (!NT_SUCCESS(Status) || !Pdpte.s.Present)
    {
        MyDbgPrint("Pdpte invalid");
        return Status;
    }
    if (Pdpte.s.LargePage)
    {
        PDPTE_LARGE* Pdpte_l = (PDPTE_LARGE*)&Pdpte;
        MyDbgPrint("Pdpte_l pfn=%llx", Pdpte_l->s.PageFrameNumber);
        return ReadPhysicalMemory(GET_1G_PFN_PAGE(Pdpte_l->s.PageFrameNumber) + GET_1G_PAGE_OFFSET(VA), Buffer, BufferSize, ReadBytes);
    }
    MyDbgPrint("Pdpte pfn=%llx", Pdpte.s.PageFrameNumber);

    Status = ReadPhysicalMemory(GET_4K_PFN_PAGE(Pdpte.s.PageFrameNumber) + 8 * Pdi, &Pde, sizeof(Pde), RetBytes);
    if (!NT_SUCCESS(Status) || !Pde.s.Present)
    {
        MyDbgPrint("Pde invalid");
        return Status;
    }
    if (Pde.s.LargePage)
    {
        PDE_LARGE* Pde_l = (PDE_LARGE*)&Pde;
        MyDbgPrint("Pde_l pfn=%llx", Pde_l->s.PageFrameNumber);
        return ReadPhysicalMemory(GET_2M_PFN_PAGE(Pde_l->s.PageFrameNumber) + GET_2M_PAGE_OFFSET(VA), Buffer, BufferSize, ReadBytes);
    }
    MyDbgPrint("Pde pfn=%llx", Pde.s.PageFrameNumber);

    Status = ReadPhysicalMemory(GET_4K_PFN_PAGE(Pde.s.PageFrameNumber) + 8 * Pti, &Pte, sizeof(Pte), RetBytes);
    if (!NT_SUCCESS(Status) || !Pte.s.Present)
    {
        MyDbgPrint("Pte invalid");
        return Status;
    }
    MyDbgPrint("Pte pfn=%llx", Pte.s.PageFrameNumber);
    return ReadPhysicalMemory(GET_4K_PFN_PAGE(Pte.s.PageFrameNumber) + GET_4K_PAGE_OFFSET(VA), Buffer, BufferSize, ReadBytes);
}


NTSTATUS GetPhysicalMem(
    _Inout_ PVOID SystemBuffer,
    _In_ ULONG InputBufferLength,
    _In_ ULONG OutputBufferLength,
    _Out_ UINT64& ReturnLength
)
{
    UNREFERENCED_PARAMETER(SystemBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    VMInfo* Input = (VMInfo*)SystemBuffer;

    if (!SystemBuffer || InputBufferLength < sizeof(VMInfo))
    {
        ReturnLength = 0;
        return STATUS_INVALID_PARAMETER;
    }

    UINT64 cr3 = Input->m_cr3;
    UINT64 eptp = Input->m_eptp;

    return ParsePageTableSelf(cr3, eptp, SystemBuffer, OutputBufferLength, ReturnLength);
}