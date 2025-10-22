#pragma once

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <fltKernel.h>

#ifdef __cplusplus
}
#endif // __cplusplus


VOID NTAPI MyDbgPrint(
	IN PCSTR Format,
	IN ...
)
{
	va_list ArgList;

	va_start(ArgList, Format);

	vDbgPrintExWithPrefix("[Hack VM] ",
		DPFLTR_IHVDRIVER_ID,
		DPFLTR_ERROR_LEVEL,
		Format,
		ArgList);

	va_end(ArgList);
}


typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT DeviceObject;
	UNICODE_STRING DeviceName;   // use for r0
	UNICODE_STRING SymLinkName;  // use for r3
} DEVICE_EXTERNSION, * PDEVICE_EXTENSION;


UNICODE_STRING g_DeviceName = RTL_CONSTANT_STRING(L"\\Device\\HackVMDev0");
UNICODE_STRING g_SymLinkName = RTL_CONSTANT_STRING(L"\\??\\HackVMDev0");


#define IOCTL_BUFFERED_DEMO \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x700, METHOD_BUFFERED, FILE_ANY_ACCESS)


UNICODE_STRING g_vboxDeviceName = RTL_CONSTANT_STRING(L"\\Device\\VBoxDrv");
PFILE_OBJECT g_vboxFileObject = NULL;
PDEVICE_OBJECT g_vboxDeviceObject = NULL;

const int g_offset_apSessionHashTab_SUPDRVDEVEXT = 0x928;	// supdrvSessionHashTabLookup
const int g_offset_pSessionGVM_SUPDRVSESSION = 0x38;
const int g_offset_cCpus_GVM = 0x134020;	// GVMMR0RegisterVCpu
const int g_offset_aCpus_GVM = 0x150000;	// GMMR0AllocateLargePage
const int g_size_GVMCPU = 0x64000;
const int g_offset_GstCtx_VMCPU = 0x3a000;	// CPUMGetGuestEAX
const int g_offset_cr3_CPUMCTX = 0x170;	// CPUMGetGuestCR3
const int g_offset_eptp_VMCPU = 0x311d8;	// PGMGetHyperCR3	vmxHCExportGuestCR3AndCR4



NTSTATUS EnumSessionList(
	IN OUT PVOID SystemBuffer,
	IN ULONG InputBufferLength,
	IN ULONG OutputBufferLength,
	OUT PULONG ReturnLength
)
{
	UNREFERENCED_PARAMETER(SystemBuffer);
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(ReturnLength);
	/*if (!systembuffer)
	{
		returnlength = 0;
		return status_invalid_parameter;
	}*/
	//DbgPrintEx(DPFLTR_SYSTEM_ID, 0, "InputLength: %d, OutputLength: %d\n", InputLength, OutputLength);
	
	NTSTATUS Status = IoGetDeviceObjectPointer(&g_vboxDeviceName, FILE_ALL_ACCESS, &g_vboxFileObject, &g_vboxDeviceObject);
	if (!NT_SUCCESS(Status))
	{
		ReturnLength = 0;
		return Status;
	}

	PVOID* SessionHashTab = (PVOID*)((UCHAR*)g_vboxDeviceObject->DeviceExtension + g_offset_apSessionHashTab_SUPDRVDEVEXT);
	for (size_t i = 0; i < 0x1FFF; i++)
	{
		PVOID apSessionHashTab = SessionHashTab[i];
		if (!apSessionHashTab)
		{
			continue;
		}
		MyDbgPrint("Find VM Session, VM pid maybe in [%d, %d, %d]\n", i, i+0x1fff, i+0x1fff+0x1fff);
		PVOID pSessionGVM = *(PVOID*)((UCHAR*)apSessionHashTab + g_offset_pSessionGVM_SUPDRVSESSION);
		if (!pSessionGVM)
		{
			continue;
		}
		DWORD CpuNumber = *(DWORD*)((UCHAR*)pSessionGVM + g_offset_cCpus_GVM);
		MyDbgPrint("pSessionGVM=%llx, CpuNumber=%d\n", pSessionGVM, CpuNumber);
		for (size_t cpuid = 0; cpuid < CpuNumber; cpuid++)
		{
			PVOID GvmCPU = (PVOID)((UCHAR*)pSessionGVM + g_offset_aCpus_GVM + cpuid * g_size_GVMCPU);
			// MyDbgPrint("guest cpu[%d] GvmCPU=%llx\n", cpuid, GvmCPU);
			UINT64 cr3 = *(UINT64*)((UCHAR*)GvmCPU + g_offset_GstCtx_VMCPU + g_offset_cr3_CPUMCTX);
			PVOID pShwPageCR3R0 = *(PVOID*)((UCHAR*)GvmCPU + g_offset_eptp_VMCPU);
			if (!pShwPageCR3R0)
			{
				continue;
			}
			UINT64 eptp = *(UINT64*)pShwPageCR3R0;
			MyDbgPrint("guest cpu[%d] cr3=%llx eptp=%llx\n", cpuid, cr3, eptp);
		}




	}
	if (g_vboxFileObject)
	{
		ObDereferenceObject(g_vboxFileObject);
		g_vboxFileObject = NULL;
	}
	if (g_vboxDeviceObject)
	{
		g_vboxDeviceObject = NULL;
	}
	ReturnLength = 0;
	return Status;
}


