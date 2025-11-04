#include <Windows.h>
#include <winioctl.h>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <vector>
#include <algorithm>

#define PAGE_SIZE 0x1000

#define IOCTL_ENUM_SESSION_LIST \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x700, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_READ_PHYSICAL_MEMORY \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x701, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ENUM_GUEST_PROCESS_LIST \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x702, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ENUM_GUEST_PROC_VAD_LIST \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x703, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IS_GVA_VALID \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x704, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ENUM_USER_MEM_PE_LIST \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x705, METHOD_BUFFERED, FILE_ANY_ACCESS)



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
	UINT64 va;		// for test from r3 to r0
	UINT64 va2;		// for test from r3 to r0
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

bool cmp_vad(const ProcVadInfo& a, const ProcVadInfo& b)
{
	return a.startpfn < b.startpfn;
}

int TestParsePageTable(HANDLE& Handle, VMInfo& info)
{
	DWORD retLength = 0;

	BOOL ret = DeviceIoControl(Handle, IOCTL_ENUM_SESSION_LIST, NULL, 0, &info, sizeof(info), &retLength, NULL);
	if (!ret)
	{
		std::cout << "DeviceIoControl error=" << GetLastError() << std::endl;
		return FALSE;
	}
	std::cout << "cr3=" << std::hex << info.cr3 << ", eptp=" << info.eptp << std::dec << std::endl;

	UCHAR buffer[0x10] = { 0 };
	ret = DeviceIoControl(Handle, IOCTL_READ_PHYSICAL_MEMORY, &info, sizeof(info), buffer, sizeof(buffer), &retLength, NULL);
	if (!ret)
	{
		std::cout << "DeviceIoControl error=" << GetLastError() << std::endl;
		return FALSE;
	}
	if (retLength != sizeof(buffer))
	{
		std::cout << "read cr3 error, size=" << retLength << std::endl;
		return FALSE;
	}
	std::cout << "read test: " << std::hex;
	for (size_t i = 0; i < sizeof(buffer)/sizeof(buffer[0]); i++)
	{
		std::cout << (int)(buffer[i] & 0xFF) << ' ';
	}
	std::cout << std::dec << std::endl;
	return TRUE;
}

int TestEnumKernelMem(const char *target, HANDLE& Handle, VMInfo& info, ProcBasicInfo& proc)
{
	std::vector<ProcBasicInfo> procList(1024);
	DWORD retLength = 0;
	BOOL ret = DeviceIoControl(Handle, IOCTL_ENUM_GUEST_PROCESS_LIST, &info, sizeof(info), procList.data(), sizeof(ProcBasicInfo)*procList.capacity(), &retLength, NULL);
	if (!ret)
	{
		std::cout << "DeviceIoControl error=" << GetLastError() << std::endl;
		return FALSE;
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
			return TRUE;
		}
	}
	return FALSE;
}

int TestProcVadTree(HANDLE& Handle, ProcBasicInfo& proc, std::vector<ProcVadInfo>& vadList)
{
	DWORD retLength = 0;
	BOOL ret = DeviceIoControl(Handle, IOCTL_ENUM_GUEST_PROC_VAD_LIST, &proc, sizeof(proc), vadList.data(), sizeof(ProcVadInfo) * vadList.capacity(), &retLength, NULL);
	if (!ret)
	{
		std::cout << "DeviceIoControl error=" << GetLastError() << std::endl;
		return FALSE;
	}
	size_t count = retLength / sizeof(ProcVadInfo);
	std::cout << "vad count=" << count << std::endl;
	vadList.resize(count);
	std::sort(vadList.begin(), vadList.end(), cmp_vad);

	return TRUE;
}

int TestProcModuleList(HANDLE& Handle, ProcBasicInfo& proc, std::vector<ProcVadInfo>& vadList)
{
	DWORD retLength = 0;
	std::vector<ProcVadInfo>::iterator it = vadList.begin();
	if (it == vadList.end())
	{
		return FALSE;
	}
	proc.va = it->startpfn << 12;
	std::vector<UINT64> PeList(1024);
	BOOL ret = DeviceIoControl(Handle, IOCTL_ENUM_USER_MEM_PE_LIST, &proc, sizeof(proc), PeList.data(), PeList.capacity() * sizeof(UINT64), &retLength, NULL);
	if (!ret)
	{
		std::cout << "DeviceIoControl error=" << GetLastError() << std::endl;
		return FALSE;
	}
	size_t count = retLength / sizeof(UINT64);
	std::cout << "pe list count=" << count << std::endl;
	PeList.resize(count);
	for (std::vector<UINT64>::iterator it = PeList.begin(); it != PeList.end(); it++)
	{
		std::cout << "pe va=" << std::hex << *it << std::endl;
	}
	return TRUE;
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
	const char* name = "notepad.exe";
	ProcBasicInfo proc = { 0 };
	if (!TestEnumKernelMem(name, Handle, info, proc))
	{
		CloseHandle(Handle);
		return -1;
	}
	std::cout << name << ": pid = " << std::hex << proc.pid << ", cr3 = " << proc.cr3 << ", eprocess = " << proc.eprocess << std::dec << ", name = " << proc.name << std::endl;

	std::vector<ProcVadInfo> vadList(1024);
	if (!TestProcVadTree(Handle, proc, vadList))
	{
		CloseHandle(Handle);
		return -1;
	}
	for (std::vector<ProcVadInfo>::iterator it = vadList.begin(); it != vadList.end(); it++)
	{
		std::cout << std::hex << "startpfn=" << it->startpfn << ", endpfn=" << it->endingpfn << ", private=" << it->isprivate << ", protection=" << it->protection << ", vadtype=" << it->vadtype << ", commitsize=" << it->commitsize << std::endl;
	}
	if (!TestProcModuleList(Handle, proc, vadList))
	{
		CloseHandle(Handle);
		return -1;
	}


	/*for (std::vector<ProcVadInfo>::iterator it = vadList.begin(); it != vadList.end(); it++)
	{
		std::cout << std::hex << "startpfn=" << it->startpfn << ", endpfn=" << it->endingpfn << ", private=" << it->isprivate << ", protection=" << it->protection << ", vadtype=" << it->vadtype << ", commitsize=" << it->commitsize << std::endl;

		size_t j = it->startpfn;
		for (; j < it->endingpfn;)
		{
			proc.va = j << 12;
			UINT64 flag1 = 0;
			ret = DeviceIoControl(Handle, IOCTL_IS_GVA_VALID, &proc, sizeof(proc), &flag1, sizeof(flag1), &retLength, NULL);
			if (!ret)
			{
				std::cout << "DeviceIoControl error=" << GetLastError() << std::endl;
				return 0;
			}
			if (!flag1)
			{
				continue;
			}
			size_t k = j+1;
			for (; k < it->endingpfn; k++)
			{
				proc.va = k << 12;
				UINT64 flag2 = 0;
				ret = DeviceIoControl(Handle, IOCTL_IS_GVA_VALID, &proc, sizeof(proc), &flag2, sizeof(flag2), &retLength, NULL);
				if (!ret)
				{
					std::cout << "DeviceIoControl error=" << GetLastError() << std::endl;
					return 0;
				}
				if (!flag2)
				{
					break;
				}
			}
			std::cout << "addr=" << proc.va << " is valid, size=" << std::dec << (k-j) * 4 << std::endl;
			j = k + 1;
		}
	}*/
	

	CloseHandle(Handle);
	return 0;
}
