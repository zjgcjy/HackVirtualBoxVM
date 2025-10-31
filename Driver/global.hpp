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

typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT DeviceObject;
	UNICODE_STRING DeviceName;   // use for r0
	UNICODE_STRING SymLinkName;  // use for r3
} DEVICE_EXTERNSION, * PDEVICE_EXTENSION;


struct VMInfo
{
	UINT64 cr3;
	UINT64 eptp;
};

struct ProcList
{
	UINT64 cr3;
	UINT64 eptp;
	PVOID eprocess;
	UINT64 pid;
	CHAR name[16];
};

extern UNICODE_STRING g_DeviceName;
extern UNICODE_STRING g_SymLinkName;


extern UNICODE_STRING g_vboxDeviceName;
extern PFILE_OBJECT g_vboxFileObject;
extern PDEVICE_OBJECT g_vboxDeviceObject;

#define IOCTL_ENUM_SESSION_LIST \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x700, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_READ_PHYSICAL_MEMORY \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x701, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ENUM_GUEST_PROCESS_LIST \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x702, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ENUM_GUEST_PROC_VAD_LIST \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x703, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef NTSTATUS(*fpMmCopyMemory)(PVOID TargetAddress, MM_COPY_ADDRESS SourceAddress, SIZE_T NumberOfBytes, ULONG Flags, PSIZE_T NumberOfBytesTransferred);

extern fpMmCopyMemory g_MmCopyMemory;

extern const UINT64 g_KernelSpaceBase;
extern const UINT64 g_KernelModulesMappingBase;   // VmmWinInit_FindNtosScan64
extern const UINT64 g_KernelModulesMappingEnd;    // 32G

extern const int g_offset_apSessionHashTab_SUPDRVDEVEXT;	// supdrvSessionHashTabLookup
extern const int g_offset_pSessionGVM_SUPDRVSESSION;
extern const int g_offset_cCpus_GVM;		// GVMMR0RegisterVCpu
extern const int g_offset_aCpus_GVM;		// GMMR0AllocateLargePage
extern const int g_size_GVMCPU;
extern const int g_offset_GstCtx_VMCPU;	// CPUMGetGuestEAX
extern const int g_offset_cr3_CPUMCTX;		// CPUMGetGuestCR3
extern const int g_offset_eptp_VMCPU;		// PGMGetHyperCR3	vmxHCExportGuestCR3AndCR4


struct WinRelatedData {
	int PsActiveProcessHead_Ntoskrnl;	// PsGetNextProcess
	int UniqueProcessId_EPROCESS;	// dt _EPROCESS UniqueProcessId
	int ActiveProcessLinks_EPROCESS;	// dt _EPROCESS ActiveProcessLinks
	int ImageFileName_EPROCESS;	// dt _EPROCESS ImageFileName
	int DirectoryTableBase_KPROCESS;	// dt _KPROCESS DirectoryTableBase
	int VadRoot_EPROCESS;	// dt _EPROCESS VadRoot
};

extern WinRelatedData g_offset_1903_18363_Nt18362;


VOID NTAPI MyDbgPrint(
	_In_ PCSTR Format,
	_In_ ...
);

#define MyDbgPrintEx(fmt, ...) \
    MyDbgPrint("%s(%d): " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

