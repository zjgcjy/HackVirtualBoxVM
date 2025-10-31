#include <Windows.h>
#include <winioctl.h>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <vector>

#define IOCTL_ENUM_SESSION_LIST \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x700, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_READ_PHYSICAL_MEMORY \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x701, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ENUM_GUEST_PROCESS_LIST \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x702, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ENUM_GUEST_PROC_VAD_LIST \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x703, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct VMInfo
{
	UINT64 cr3;
	UINT64 eptp;
};

struct ProcBasicInfo
{
	UINT64 cr3;
	UINT64 eptp;
	PVOID eprocess;
	UINT64 pid;
	CHAR name[16];
};

struct ProcVadInfo
{
	UINT64 startpfn;
	UINT64 endingpfn;
	UINT64 level;
	UINT64 protection;
	UINT64 vadtype;
	UINT64 commitsize;
	UINT64 isprivate;
};

int TestParsePageTable(HANDLE& Handle, VMInfo& info)
{
	DWORD retLength = 0;

	BOOL ret = DeviceIoControl(Handle, IOCTL_ENUM_SESSION_LIST, NULL, 0, &info, sizeof(info), &retLength, NULL);
	if (!ret)
	{
		std::cout << "DeviceIoControl error=" << GetLastError() << std::endl;
		return 0;
	}
	std::cout << "cr3=" << std::hex << info.cr3 << ", eptp=" << info.eptp << std::dec << std::endl;

	UCHAR buffer[0x10] = { 0 };
	ret = DeviceIoControl(Handle, IOCTL_READ_PHYSICAL_MEMORY, &info, sizeof(info), buffer, sizeof(buffer), &retLength, NULL);
	if (!ret)
	{
		std::cout << "DeviceIoControl error=" << GetLastError() << std::endl;
		return 0;
	}
	if (retLength != sizeof(buffer))
	{
		std::cout << "read cr3 error, size=" << retLength << std::endl;
		return 0;
	}
	std::cout << "read test: " << std::hex;
	for (size_t i = 0; i < sizeof(buffer)/sizeof(buffer[0]); i++)
	{
		std::cout << (int)(buffer[i] & 0xFF) << ' ';
	}
	std::cout << std::dec << std::endl;
	return 1;
}

int TestEnumKernelMem(const char *target, HANDLE& Handle, VMInfo& info, ProcBasicInfo& proc)
{
	std::vector<ProcBasicInfo> procList(1024);
	DWORD retLength = 0;
	BOOL ret = DeviceIoControl(Handle, IOCTL_ENUM_GUEST_PROCESS_LIST, &info, sizeof(info), procList.data(), sizeof(ProcBasicInfo)*procList.capacity(), &retLength, NULL);
	if (!ret)
	{
		std::cout << "DeviceIoControl error=" << GetLastError() << std::endl;
		return 0;
	}
	size_t count = retLength / sizeof(ProcBasicInfo);
	std::cout << "proc count=" << count << std::endl;
	procList.resize(count);
	for (std::vector<ProcBasicInfo>::iterator it = procList.begin(); it != procList.end(); it++)
	{
		// std::cout << "pid=" << std::hex << it->pid << ", cr3=" << it->cr3 << std::dec << ", name=" << it->name << std::endl;
		if (!strcmp(it->name, target))
		{
			proc.cr3 = it->cr3;
			proc.eptp = info.eptp;
			proc.pid = it->pid;
			proc.eprocess = it->eprocess;
			memcpy(proc.name, it->name, sizeof(proc.name));
			return 1;
		}
	}
	return 0;
}

int TestProcVadTree(HANDLE& Handle, ProcBasicInfo& proc)
{
	std::vector<ProcVadInfo> vadList(1024);
	DWORD retLength = 0;
	BOOL ret = DeviceIoControl(Handle, IOCTL_ENUM_GUEST_PROC_VAD_LIST, &proc, sizeof(proc), vadList.data(), sizeof(ProcVadInfo) * vadList.capacity(), &retLength, NULL);
	if (!ret)
	{
		std::cout << "DeviceIoControl error=" << GetLastError() << std::endl;
		return 0;
	}
	size_t count = retLength / sizeof(ProcVadInfo);
	std::cout << "vad count=" << count << std::endl;
	vadList.resize(count);
	for (std::vector<ProcVadInfo>::iterator it = vadList.begin(); it != vadList.end(); it++)
	{
		std::cout << std::hex << "startpfn=" << it->startpfn << ", endpfn=" << it->endingpfn << ", private=" << it->isprivate << ", protection=" << it->protection << ", vadtype=" << it->vadtype << ", commitsize=" << it->commitsize << ", level=" << it->level << std::endl;
	}

	return 0;
}

int main()
{
	HANDLE Handle = CreateFile(L"\\\\.\\HackVMDev0", GENERIC_ALL, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	BOOL ret = FALSE;
	if (Handle == INVALID_HANDLE_VALUE)
	{
		std::cout << "CreateFile error=" << GetLastError() << std::endl;
		return -1;
	}
	VMInfo info = { 0 };
	if (!TestParsePageTable(Handle, info))
	{
		CloseHandle(Handle);
		return -1;
	}
	ProcBasicInfo proc = { 0 };
	if (!TestEnumKernelMem("notepad.exe", Handle, info, proc))
	{
		CloseHandle(Handle);
		return -1;
	}
	std::cout << "notepad pid=" << std::hex << proc.pid << ", cr3=" << proc.cr3 << ", eprocess=" << proc.eprocess << std::dec << ", name=" << proc.name << std::endl;
	if (!TestProcVadTree(Handle, proc))
	{
		CloseHandle(Handle);
		return -1;
	}


	CloseHandle(Handle);
	return 0;
}