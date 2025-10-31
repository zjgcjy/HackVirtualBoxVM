#pragma once

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
#include <fltKernel.h>
#include <minwindef.h>
#include <windef.h>
#include <ntstrsafe.h>
#include <ntimage.h>
#include <intrin.h>
#include <ntifs.h>
#ifdef __cplusplus
}
#endif // __cplusplus


//0x8 bytes (sizeof)
struct _EX_PUSH_LOCK
{
    union
    {
        //struct
        //{
        //    ULONGLONG Locked : 1;                                             //0x0
        //    ULONGLONG Waiting : 1;                                            //0x0
        //    ULONGLONG Waking : 1;                                             //0x0
        //    ULONGLONG MultipleShared : 1;                                     //0x0
        //    ULONGLONG Shared : 60;                                            //0x0
        //};
        ULONGLONG Value;                                                    //0x0
        VOID* Ptr;                                                          //0x0
    };
};

//0x4 bytes (sizeof)
struct _MMVAD_FLAGS
{
    ULONG Lock : 1;                                                           //0x0
    ULONG LockContended : 1;                                                  //0x0
    ULONG DeleteInProgress : 1;                                               //0x0
    ULONG NoChange : 1;                                                       //0x0
    ULONG VadType : 3;                                                        //0x0
    ULONG Protection : 5;                                                     //0x0
    ULONG PreferredNode : 7;                                                  //0x0
    ULONG PageSize : 2;                                                       //0x0
    ULONG PrivateMemory : 1;                                                  //0x0
};

//0x4 bytes (sizeof)
struct _MMVAD_FLAGS1
{
    ULONG CommitCharge : 31;                                                  //0x0
    ULONG MemCommit : 1;                                                      //0x0
};

//0x4 bytes (sizeof)
struct _MM_PRIVATE_VAD_FLAGS
{
    ULONG Lock : 1;                                                           //0x0
    ULONG LockContended : 1;                                                  //0x0
    ULONG DeleteInProgress : 1;                                               //0x0
    ULONG NoChange : 1;                                                       //0x0
    ULONG VadType : 3;                                                        //0x0
    ULONG Protection : 5;                                                     //0x0
    ULONG PreferredNode : 7;                                                  //0x0
    ULONG PageSize : 2;                                                       //0x0
    ULONG PrivateMemoryAlwaysSet : 1;                                         //0x0
    ULONG WriteWatch : 1;                                                     //0x0
    ULONG FixedLargePageSize : 1;                                             //0x0
    ULONG ZeroFillPagesOptional : 1;                                          //0x0
    ULONG Graphics : 1;                                                       //0x0
    ULONG Enclave : 1;                                                        //0x0
    ULONG ShadowStack : 1;                                                    //0x0
    ULONG PhysicalMemoryPfnsReferenced : 1;                                   //0x0
};

//0x4 bytes (sizeof)
struct _MM_SHARED_VAD_FLAGS
{
    ULONG Lock : 1;                                                           //0x0
    ULONG LockContended : 1;                                                  //0x0
    ULONG DeleteInProgress : 1;                                               //0x0
    ULONG NoChange : 1;                                                       //0x0
    ULONG VadType : 3;                                                        //0x0
    ULONG Protection : 5;                                                     //0x0
    ULONG PreferredNode : 7;                                                  //0x0
    ULONG PageSize : 2;                                                       //0x0
    ULONG PrivateMemoryAlwaysClear : 1;                                       //0x0
    ULONG PrivateFixup : 1;                                                   //0x0
    ULONG HotPatchState : 2;                                                  //0x0
};

//0x88 bytes (sizeof)
typedef struct _MMVAD_SHORT
{
    struct _RTL_BALANCED_NODE VadNode;                                      //0x00
    ULONG StartingVpn;                                                      //0x18
    ULONG EndingVpn;                                                        //0x1c
    UCHAR StartingVpnHigh;                                                  //0x20
    UCHAR EndingVpnHigh;                                                    //0x21
    UCHAR CommitChargeHigh;                                                 //0x22
    UCHAR SpareNT64VadUChar;                                                //0x23
    LONG ReferenceCount;                                                    //0x24
    struct _EX_PUSH_LOCK PushLock;                                          //0x28
    union
    {
        ULONG LongFlags;                                                    //0x30
        struct _MMVAD_FLAGS VadFlags;                                       //0x30
        struct _MM_PRIVATE_VAD_FLAGS PrivateVadFlags;                       //0x30
        //struct _MM_GRAPHICS_VAD_FLAGS GraphicsVadFlags;                     //0x30
        struct _MM_SHARED_VAD_FLAGS SharedVadFlags;                         //0x30
        volatile ULONG VolatileVadLong;                                     //0x30
    } u;                                                                    //0x30
    union
    {
        ULONG LongFlags1;                                                   //0x34
        struct _MMVAD_FLAGS1 VadFlags1;                                     //0x34
    } u1;
} MMVAD_SHORT, * PMMVAD_SHORT;



NTSTATUS EnumVadTree(
    _In_ UINT64 CR3,
    _In_ UINT64 EPTP,
    _In_ PVOID VadRoot
    //_Inout_ PVOID Buffer,
    //_In_ UINT64 BufferSize,
    //_Out_ UINT64& ReturnLength
);

