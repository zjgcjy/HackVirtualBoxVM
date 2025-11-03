#include "aob.hpp"
#include "mm.hpp"

BOOL AobSearcher(
    _In_ PCHAR Buffer,
    _In_ SIZE_T BufferSize,
    _In_ const PCHAR Pattern,
    _In_ const PCHAR Mask,
    _Out_ SIZE_T& Offset
)
{
    Offset = 0;
    SIZE_T Size = strlen(Mask);
    if (Buffer == NULL || Pattern == NULL || Mask == NULL || Size == 0 || BufferSize < Size)
    {
        return FALSE;
    }
    for (size_t i = 0; i <= BufferSize - Size; i++)
    {
        bool matched = true;
        for (size_t j = 0; j < Size; j++)
        {
            if (Mask[j] == '?')
            {
                continue;
            }
            // mask[j] == 'x'
            if (Buffer[i + j] != Pattern[j])
            {
                matched = false;
                break;
            }
        }
        if (matched)
        {
            Offset = i;
            return TRUE;
        }
    }
    return FALSE;
}


BOOL IsGPANtoskrnlBase(
    _In_ UINT64 GPA,
    _In_ UINT64 EPTP,
    _In_ SIZE_T Size
)
{
    const ULONG tag = 'SMTG';
    BOOL Status = FALSE;
    PVOID Buffer = ExAllocatePoolZero(PagedPool, Size, tag);
    if (Buffer == NULL || Size > PAGE_SIZE_1G)
    {
        return Status;
    }
    do
    {
        UINT64 RetBytes = 0;
        if (!NT_SUCCESS(ReadGPA(GPA, EPTP, Buffer, Size, RetBytes)))
        {
            break;
        }

        SIZE_T Offset = 0;
        // check pe dos
        if (!AobSearcher((CHAR*)Buffer, Size, "\x4d\x5a\x90\x00\x03\x00\x00\x00\x04\x00\x00\x00\xff\xff\x00\x00", "xxxxxxxxxxxxxxxx", Offset))
        {
            break;
        }
        PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)Buffer;
        if (Offset != 0 || DosHeader->e_magic != 'ZM' || DosHeader->e_lfanew >= Size)
        {
            break;
        }
        PIMAGE_NT_HEADERS NtHeader = (PIMAGE_NT_HEADERS)((PCHAR)Buffer + DosHeader->e_lfanew);
        if (NtHeader->Signature != 'EP')
        {
            break;
        }
        USHORT NumberOfSections = NtHeader->FileHeader.NumberOfSections;
        PIMAGE_SECTION_HEADER SecHeader = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader + sizeof(*NtHeader));

        BOOL flag = FALSE;
        for (size_t i = 0; i < NumberOfSections; i++, SecHeader++)
        {
            if (*(UINT64*)(SecHeader->Name) == 0x45444F434C4F4F50) // POOLCODE
            {
                flag = TRUE;
                break;
            }
        }
        if (!flag)
        {
            break;
        }

        Status = TRUE;
    } while (FALSE);
    if (Buffer)
    {
        ExFreePoolWithTag(Buffer, tag);
        Buffer = NULL;
    }
    return Status;
}


BOOL IsGPAPeBase(
    _In_ UINT64 GPA,
    _In_ UINT64 EPTP,
    _In_ SIZE_T Size
)
{
    const ULONG tag = 'SMTG';
    BOOL Status = FALSE;
    PVOID Buffer = ExAllocatePoolZero(PagedPool, Size, tag);
    if (Buffer == NULL || Size > PAGE_SIZE_1G)
    {
        return Status;
    }
    do
    {
        UINT64 RetBytes = 0;
        if (!NT_SUCCESS(ReadGPA(GPA, EPTP, Buffer, Size, RetBytes)))
        {
            break;
        }

        SIZE_T Offset = 0;
        // check pe dos
        if (!AobSearcher((CHAR*)Buffer, Size, "\x4d\x5a\x90\x00\x03\x00\x00\x00\x04\x00\x00\x00\xff\xff\x00\x00", "xxxxxxxxxxxxxxxx", Offset))
        {
            break;
        }
        PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)Buffer;
        if (Offset != 0 || DosHeader->e_magic != 'ZM' || DosHeader->e_lfanew >= Size)
        {
            break;
        }
        PIMAGE_NT_HEADERS NtHeader = (PIMAGE_NT_HEADERS)((PCHAR)Buffer + DosHeader->e_lfanew);
        if (NtHeader->Signature != 'EP')
        {
            break;
        }
        Status = TRUE;
    } while (FALSE);
    if (Buffer)
    {
        ExFreePoolWithTag(Buffer, tag);
        Buffer = NULL;
    }
    return Status;
}
