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
#include "mm.hpp"
#include "aob.hpp"

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
			Status = GetVMCpuInfo(Buffer, InputLength, OutputLength, ReturnLength);
			break;
		}
		case IOCTL_READ_PHYSICAL_MEMORY:
		{
			Status = GetPhysicalMem(Buffer, InputLength, OutputLength, ReturnLength);
			break;
		}
		case IOCTL_ENUM_GVM_KERNEL_MEMORY:
		{
			Status = EnumKernelMemory(Buffer, InputLength, OutputLength, ReturnLength);
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
	MyDbgPrint("Start DriverEntry\n");

	DriverObject->MajorFunction[IRP_MJ_CREATE] = IrpCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = IrpClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IrpIoctl;
	DriverObject->DriverUnload = NtDriverUnload;

	NTSTATUS status = NtDriverCreateDevice(DriverObject);


	MyDbgPrint("End DriverEntry\n");
	return status;
}
