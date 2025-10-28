#include "global.hpp"

UNICODE_STRING g_DeviceName = RTL_CONSTANT_STRING(L"\\Device\\HackVMDev0");
UNICODE_STRING g_SymLinkName = RTL_CONSTANT_STRING(L"\\??\\HackVMDev0");

UNICODE_STRING g_vboxDeviceName = RTL_CONSTANT_STRING(L"\\Device\\VBoxDrv");
PFILE_OBJECT g_vboxFileObject = NULL;
PDEVICE_OBJECT g_vboxDeviceObject = NULL;

fpMmCopyMemory g_MmCopyMemory = NULL;

const UINT64 g_KernelSpaceBase = 0xFFFF800000000000;
const UINT64 g_KernelModulesMappingBase = 0xFFFFF80000000000;   // VmmWinInit_FindNtosScan64
const UINT64 g_KernelModulesMappingEnd = 0xFFFFF807FFFFFFFF;    // 32G

const int g_offset_apSessionHashTab_SUPDRVDEVEXT = 0x928;	// supdrvSessionHashTabLookup
const int g_offset_pSessionGVM_SUPDRVSESSION = 0x38;
const int g_offset_cCpus_GVM = 0x134020;		// GVMMR0RegisterVCpu
const int g_offset_aCpus_GVM = 0x150000;		// GMMR0AllocateLargePage
const int g_size_GVMCPU = 0x64000;
const int g_offset_GstCtx_VMCPU = 0x3a000;	// CPUMGetGuestEAX
const int g_offset_cr3_CPUMCTX = 0x170;		// CPUMGetGuestCR3
const int g_offset_eptp_VMCPU = 0x311d8;		// PGMGetHyperCR3	vmxHCExportGuestCR3AndCR4


//const int g_offset_UniqueProcessId_EPORCESS = 0x1d0;
//const int g_offset_ActiveProcessLinks_EPORCESS = 0x1d8;
//const int g_offset_ImageFileName_EPROCESS = 0x338;
//const int g_offset_DirectoryTableBase_KPROCESS = 0x28;

WinRelatedData g_offset_1903_18363_Nt18362 = { 0x438b40, 0x2e8, 0x2f0, 0x450, 0x28};

VOID NTAPI MyDbgPrint(
	_In_ PCSTR Format,
	_In_ ...
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
