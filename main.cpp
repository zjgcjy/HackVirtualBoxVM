

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


EXTERN_C NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistyPath
)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistyPath);
	MyDbgPrint("Start DriverEntry\n");

	DriverObject->MajorFunction[IRP_MJ_CREATE] = IrpCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = IrpClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = NtDriverIRP;
	DriverObject->DriverUnload = NtDriverUnload;

	NTSTATUS status = NtDriverCreateDevice(DriverObject);

	MyDbgPrint("End DriverEntry\n");
	return status;
}
