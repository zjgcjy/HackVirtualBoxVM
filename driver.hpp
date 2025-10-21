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
	//#include <ntddk.h>
	//#include <ntifs.h>
	//#include <wdm.h>
	//#include <winnt.h>

#ifdef __cplusplus
}
#endif // __cplusplus

#include "driver.hpp"


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


#define IOCTL_BUFFERED_DEMO \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x700, METHOD_BUFFERED, FILE_ANY_ACCESS)


const int g_offset_apSessionHashTab = 0x928;
UNICODE_STRING g_vbox_device = RTL_CONSTANT_STRING(L"\\Device\\VBoxDrv");
PFILE_OBJECT g_vboxFileObject = NULL;
PDEVICE_OBJECT g_vboxDeviceObject = NULL;
const int g_offset_pSessionGVM_SUPDRVSESSION = 0x38;
const int g_offset_cCpus_GVM = 0x134020;
const int g_offset_aCpus_GVM = 0x150000;
const int g_size_GVMCPU = 0x64000;

VOID NtDriverUnload(
	_In_ PDRIVER_OBJECT DriverObject
)
{
	MyDbgPrint("Start NtDriverUnload\n");
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
	MyDbgPrint("End NtDriverUnload\n");
}

NTSTATUS IrpCreate(
	_In_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
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
	UNREFERENCED_PARAMETER(Irp);
	NTSTATUS Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

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
	
	NTSTATUS Status = IoGetDeviceObjectPointer(&g_vbox_device, FILE_ALL_ACCESS, &g_vboxFileObject, &g_vboxDeviceObject);
	if (!NT_SUCCESS(Status))
	{
		ReturnLength = 0;
		return Status;
	}

	PVOID* SessionHashTab = (PVOID*)((UCHAR*)g_vboxDeviceObject->DeviceExtension + g_offset_apSessionHashTab);
	for (size_t i = 0; i < 0x1FFF; i++)
	{
		PVOID apSessionHashTab = SessionHashTab[i];
		if (!apSessionHashTab)
		{
			continue;
		}
		MyDbgPrint("Find Session pid=[%d, %d, %d]\n", i, i+0x1fff, i+0x1fff+0x1fff);
		PVOID pSessionGVM = *(PVOID*)((UCHAR*)apSessionHashTab + g_offset_pSessionGVM_SUPDRVSESSION);
		if (!pSessionGVM)
		{
			continue;
		}
		MyDbgPrint("pSessionGVM = %llx\n", pSessionGVM);




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

NTSTATUS NtDriverIRP(
	_In_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP Irp
)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	MyDbgPrint("Start NtDriverIRP\n");

	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG InputLength = Stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputLength = Stack->Parameters.DeviceIoControl.OutputBufferLength;
	PUCHAR Buffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;

	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	ULONG ReturnLength = 0;
	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_BUFFERED_DEMO:
		{
			Status = EnumSessionList(Buffer, InputLength, OutputLength, &ReturnLength);
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
	MyDbgPrint("End NtDriverIRP\n");
	return Status;
}

NTSTATUS NtDriverCreateDevice(
	_In_ PDRIVER_OBJECT DriverObject
)
{
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\HackVMDev0");
	// create device object by IoCreateDevice
	PDEVICE_OBJECT Deviceobject;
	NTSTATUS status = IoCreateDevice(DriverObject, sizeof(_DEVICE_EXTENSION), &DeviceName, FILE_DEVICE_UNKNOWN, 0, TRUE, &Deviceobject);
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	Deviceobject->Flags |= DO_BUFFERED_IO;

	UNICODE_STRING SymLinkName = RTL_CONSTANT_STRING(L"\\??\\HackVMDev0");
	status = IoCreateSymbolicLink(&SymLinkName, &DeviceName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(Deviceobject);
		return status;
	}
	// init device extension
	PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)Deviceobject->DeviceExtension;
	DeviceExtension->DeviceObject = Deviceobject;
	DeviceExtension->DeviceName = DeviceName;
	DeviceExtension->SymLinkName = SymLinkName;
	return status;
}



