#include "global.hpp"
#include "vad.hpp"
#include "mm.hpp"


VOID EnumVad(
    _In_ UINT64 CR3,
    _In_ UINT64 EPTP,
    _In_ PVOID Addr,
    _Out_ UINT64 &Count,
    _In_ UINT64 Level
)
{
    if (Addr == 0)
    {
        return;
    }
    UINT64 ReadBytes = 0;
    RTL_BALANCED_NODE node = { 0 };
    NTSTATUS Status = ReadGVA((UINT64)Addr, CR3, EPTP, &node, sizeof(node), ReadBytes);
    if (!NT_SUCCESS(Status))
    {
        return;
    }
    Count += 1;
    MyDbgPrintEx("node=%p, left=%p, right=%p, count=%d, level=%d", Addr, node.Left, node.Right, Count, Level);
    if (node.Left != 0)
    {
        EnumVad(CR3, EPTP, node.Left, Count, Level + 1);
    }
    if (node.Right != 0)
    {
        EnumVad(CR3, EPTP, node.Right, Count, Level + 1);
    }
    return;
}


NTSTATUS EnumVadTree(
    _In_ UINT64 CR3,
    _In_ UINT64 EPTP,
    _In_ PVOID VadRoot
    //_Inout_ PVOID Buffer,
    //_In_ UINT64 BufferSize,
    //_Out_ UINT64& ReturnLength
)
{
    //ReturnLength = 0;
    if (!VadRoot)
    {
        return STATUS_UNSUCCESSFUL;
    }
    UINT64 count = 0;
    EnumVad(CR3, EPTP, VadRoot, count, 0);
    return STATUS_SUCCESS;
}
