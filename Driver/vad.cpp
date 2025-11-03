#include "global.hpp"
#include "vad.hpp"
#include "mm.hpp"


VOID EnumVad(
    _In_ UINT64 CR3,
    _In_ UINT64 EPTP,
    _In_ PVOID Addr,
    _Inout_ UINT64 &Count,
    _In_ UINT64 Level,
    _Inout_ ProcVadInfo*& Buffer,
    _In_ UINT64 BufferSize
)
{
    if (!Addr || !Buffer)
    {
        return;
    }
    UINT64 ReadBytes = 0;
    BYTE temp[0x100] = { 0 };
    MMVAD_SHORT* vad = (MMVAD_SHORT*)temp;
    RTL_BALANCED_NODE* node = &vad->VadNode;

    NTSTATUS Status = ReadGVA((UINT64)Addr, CR3, EPTP, temp, sizeof(temp), ReadBytes);
    if (!NT_SUCCESS(Status))
    {
        return;
    }
    Count += 1;
    if (Count * sizeof(ProcBasicInfo) > BufferSize)
    {
        return;
    }
    UINT64 StartingVpn = (UINT64)vad->StartingVpnHigh << 32;
    StartingVpn |= vad->StartingVpn;
    UINT64 EndingVpn = (UINT64)vad->EndingVpnHigh << 32;
    EndingVpn |= vad->EndingVpn;
    EndingVpn += 1;

    ULONG Protection = vad->u.VadFlags.Protection;
    ULONG VadType = vad->u.VadFlags.VadType;
    ULONG CommitCharge = vad->u1.VadFlags1.CommitCharge;
    ULONG PrivateMemory = vad->u.VadFlags.PrivateMemory;

    MyDbgPrintEx("node=%p, level=%d, start=%llx, end=%llx, Protection=%d, VadType=%d, Commit=%d, Private=%d", Addr, Level, StartingVpn, EndingVpn, Protection, VadType, CommitCharge, PrivateMemory);

    Buffer->startpfn = StartingVpn;
    Buffer->endingpfn = EndingVpn;
    Buffer->level = Level;
    Buffer->protection = Protection;
    Buffer->vadtype = VadType;
    Buffer->commitsize = CommitCharge;
    Buffer->isprivate = PrivateMemory;
    Buffer++;

    if (node->Left != 0)
    {
        EnumVad(CR3, EPTP, node->Left, Count, Level + 1, Buffer, BufferSize);
    }
    if (node->Right != 0)
    {
        EnumVad(CR3, EPTP, node->Right, Count, Level + 1, Buffer, BufferSize);
    }
    return;
}

NTSTATUS EnumVadTree(
    _In_ const WinRelatedData& Offset,
    _In_ const PEPROCESS Process,
    _In_ UINT64 CR3,
    _In_ UINT64 EPTP,
    _Inout_ ProcVadInfo* Buffer,
    _In_ UINT64 BufferSize,
    _Out_ UINT64& ReturnLength
)
{
    ReturnLength = 0;
    if (!Buffer || !Process)
    {
        return STATUS_INVALID_PARAMETER_1;
    }
    UINT64 ReadBytes = 0;
    PVOID Root = NULL;
    NTSTATUS Status = ReadGVA((UINT64)((PCHAR)Process + Offset.VadRoot_EPROCESS), CR3, EPTP, &Root, sizeof(Root), ReadBytes);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    if (!Root)
    {
        return STATUS_UNSUCCESSFUL;
    }

    MyDbgPrintEx("VadRoot=%p", Root);
    UINT64 count = 0;
    EnumVad(CR3, EPTP, Root, count, 0, Buffer, BufferSize);
    MyDbgPrintEx("Vad Count=%d", count);
    ReturnLength = count * sizeof(ProcVadInfo);
    return STATUS_SUCCESS;
}

