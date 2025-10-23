#include <Windows.h>
#include <winioctl.h>
#include <iostream>

#define IOCTL_ENUM_SESSION_LIST \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x700, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_READ_PHYSICAL_MEMORY \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x701, METHOD_BUFFERED, FILE_ANY_ACCESS)


struct VMInfo
{
	UINT64 m_cr3;
	UINT64 m_eptp;
};


int TEST(HANDLE& Handle)
{
	DWORD retLength = 0;

	VMInfo info = { 0 };
	BOOL ret = DeviceIoControl(Handle, IOCTL_ENUM_SESSION_LIST, NULL, 0, &info, sizeof(info), &retLength, NULL);
	if (!ret)
	{
		std::cout << "DeviceIoControl error: " << GetLastError() << std::endl;
		return 0;
	}
	std::cout << "cr3=" << std::hex << info.m_cr3 << ", eptp=" << info.m_eptp << std::dec << std::endl;


	UCHAR buffer[0x10] = { 0 };

	ret = DeviceIoControl(Handle, IOCTL_READ_PHYSICAL_MEMORY, &info, sizeof(info), buffer, sizeof(buffer), &retLength, NULL);
	if (!ret)
	{
		std::cout << "DeviceIoControl error: " << GetLastError() << std::endl;
		return 0;
	}
	std::cout << "retLength=" << retLength << std::endl;
	for (size_t i = 0; i < sizeof(buffer)/sizeof(buffer[0]); i++)
	{
		std::cout << std::hex << (int)buffer[i] << std::oct << std::endl;
	}





	return 1;
}

int main()
{
	HANDLE Handle = CreateFile(L"\\\\.\\HackVMDev0", GENERIC_ALL, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	BOOL ret = FALSE;
	if (Handle == INVALID_HANDLE_VALUE)
	{
		std::cout << "CreateFile error: " << GetLastError() << std::endl;
		return -1;
	}
	if (!TEST(Handle))
	{
		CloseHandle(Handle);
		return -1;
	}



	CloseHandle(Handle);
	return 0;
}