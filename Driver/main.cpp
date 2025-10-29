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
	//#include <ntddk.h>
	//#include <ntifs.h>
	//#include <wdm.h>
	//#include <winnt.h>
#ifdef __cplusplus
}
#endif // __cplusplus

#include "mm.hpp"
#include "aob.hpp"
#include "global.hpp"

NTSTATUS IrpCreate(
	_In_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	NTSTATUS Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

NTSTATUS IrpClose(
	_In_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	NTSTATUS Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}



NTSTATUS GetGuestVMInfo(
	_Inout_ PVOID SystemBuffer,
	_In_ ULONG InputBufferLength,
	_In_ ULONG OutputBufferLength,
	_Out_ UINT64& ReturnLength
)
{
	UNREFERENCED_PARAMETER(SystemBuffer);
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBufferLength);

	VMInfo* Output = (VMInfo*)SystemBuffer;
	ReturnLength = 0;

	if (!SystemBuffer || OutputBufferLength < sizeof(VMInfo))
	{
		return STATUS_INVALID_PARAMETER;
	}
	NTSTATUS Status = IoGetDeviceObjectPointer(&g_vboxDeviceName, FILE_ALL_ACCESS, &g_vboxFileObject, &g_vboxDeviceObject);
	if (!NT_SUCCESS(Status))
	{
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
		MyDbgPrintEx("Find VM Session, VM pid maybe in [%d, %d, %d]\n", i, i + 0x1fff, i + 0x1fff + 0x1fff);
		PVOID pSessionGVM = *(PVOID*)((UCHAR*)apSessionHashTab + g_offset_pSessionGVM_SUPDRVSESSION);
		if (!pSessionGVM)
		{
			continue;
		}
		DWORD CpuNumber = *(DWORD*)((UCHAR*)pSessionGVM + g_offset_cCpus_GVM);
		MyDbgPrintEx("pSessionGVM=%llx, CpuNumber=%d\n", pSessionGVM, CpuNumber);
		for (size_t cpuid = 0; cpuid < CpuNumber; cpuid++)
		{
			PVOID GvmCPU = (PVOID)((UCHAR*)pSessionGVM + g_offset_aCpus_GVM + cpuid * g_size_GVMCPU);
			// MyDbgPrintEx("guest cpu[%d] GvmCPU=%llx\n", cpuid, GvmCPU);
			UINT64 cr3 = *(UINT64*)((UCHAR*)GvmCPU + g_offset_GstCtx_VMCPU + g_offset_cr3_CPUMCTX);
			PVOID pShwPageCR3R0 = *(PVOID*)((UCHAR*)GvmCPU + g_offset_eptp_VMCPU);
			if (!pShwPageCR3R0)
			{
				continue;
			}
			UINT64 eptp = *(UINT64*)pShwPageCR3R0;
			MyDbgPrintEx("guest cpu[%d] cr3=%llx eptp=%llx\n", cpuid, cr3, eptp);
			Output->cr3 = cr3;
			Output->eptp = eptp;
			ReturnLength = sizeof(VMInfo);
			break;
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
	return Status;
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
	ReturnLength = 0;
	if (!SystemBuffer || InputBufferLength < sizeof(VMInfo))
	{
		return STATUS_INVALID_PARAMETER;
	}

	UINT64 cr3 = Input->cr3;
	UINT64 eptp = Input->eptp;

	return ReadGPA(cr3, eptp, SystemBuffer, OutputBufferLength, ReturnLength);
}

NTSTATUS GetGuestProcessList(
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
	ReturnLength = 0;
	if (!SystemBuffer || InputBufferLength < sizeof(VMInfo))
	{
		return STATUS_INVALID_PARAMETER;
	}

	UINT64 cr3 = Input->cr3;
	UINT64 eptp = Input->eptp;
	ProcList* procBuffer = (ProcList*) SystemBuffer;

	UINT64 NtoskrnlGPA = 0;
	UINT64 NtoskrnlGVA = 0;
	NTSTATUS Status = GetNtosBaseByEnumPageTable(cr3, eptp, NtoskrnlGPA, NtoskrnlGVA);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}
	MyDbgPrintEx("NtoskrnlGPA=%llx, NtoskrnlGVA=%llx", NtoskrnlGPA, NtoskrnlGVA);

	WinRelatedData Offset = g_offset_1903_18363_Nt18362;
	Status = GetGuestProcessList(NtoskrnlGVA, Offset, cr3, eptp, procBuffer, OutputBufferLength, ReturnLength);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}
	return Status;
}

NTSTATUS IrpIoctl(
	_In_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP Irp
)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG InputLength = Stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputLength = Stack->Parameters.DeviceIoControl.OutputBufferLength;
	PUCHAR Buffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;

	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	ULONG_PTR ReturnLength = 0;

	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_ENUM_SESSION_LIST:
		{
			Status = GetGuestVMInfo(Buffer, InputLength, OutputLength, ReturnLength);
			break;
		}
		case IOCTL_READ_PHYSICAL_MEMORY:
		{
			Status = GetPhysicalMem(Buffer, InputLength, OutputLength, ReturnLength);
			break;
		}
		case IOCTL_ENUM_GUEST_PROCESS_LIST:
		{
			Status = GetGuestProcessList(Buffer, InputLength, OutputLength, ReturnLength);
			break;
		}
		default:
		{
			Status = STATUS_INVALID_DEVICE_REQUEST;
			ReturnLength = 0;
			break;
		}
	}

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = ReturnLength;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

VOID NtDriverUnload(
	_In_ PDRIVER_OBJECT DriverObject
)
{
	MyDbgPrintEx("Start NtDriverUnload\n");
	// delete device and symlink
	PDEVICE_OBJECT DeviceObject = DriverObject->DeviceObject;
	while (DeviceObject != NULL)
	{
		PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
		IoDeleteSymbolicLink(&DeviceExtension->SymLinkName);
		// delete current, move to next
		DeviceObject = DeviceObject->NextDevice;
		IoDeleteDevice(DeviceExtension->DeviceObject);
	}
	MyDbgPrintEx("End NtDriverUnload\n");
}

NTSTATUS NtDriverCreateDevice(
	_In_ PDRIVER_OBJECT DriverObject
)
{
	// create device object by IoCreateDevice
	PDEVICE_OBJECT Deviceobject;
	NTSTATUS status = IoCreateDevice(DriverObject, sizeof(_DEVICE_EXTENSION), &g_DeviceName, FILE_DEVICE_UNKNOWN, 0, TRUE, &Deviceobject);
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	Deviceobject->Flags |= DO_BUFFERED_IO;

	status = IoCreateSymbolicLink(&g_SymLinkName, &g_DeviceName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(Deviceobject);
		return status;
	}
	// init device extension
	PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)Deviceobject->DeviceExtension;
	DeviceExtension->DeviceObject = Deviceobject;
	DeviceExtension->DeviceName = g_DeviceName;
	DeviceExtension->SymLinkName = g_SymLinkName;
	return status;
}

EXTERN_C NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistyPath
)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistyPath);
	MyDbgPrintEx("Start DriverEntry\n");

	DriverObject->MajorFunction[IRP_MJ_CREATE] = IrpCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = IrpClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IrpIoctl;
	DriverObject->DriverUnload = NtDriverUnload;

	NTSTATUS status = NtDriverCreateDevice(DriverObject);


	MyDbgPrintEx("End DriverEntry\n");
	return status;
}

