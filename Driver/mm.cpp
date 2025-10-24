
#include "mm.hpp"
#include "global.hpp"

NTSTATUS ReadPhysicalMemory(
    _In_ UINT64 Addr,
    _Out_ PVOID Buffer,
    _In_ UINT64 Size,
    _Out_ UINT64& BytesTransferred
)
{
    if (g_MmCopyMemory == NULL)
    {
        UNICODE_STRING FunctionName;
        RtlInitUnicodeString(&FunctionName, L"MmCopyMemory");
        g_MmCopyMemory = (fpMmCopyMemory)MmGetSystemRoutineAddress(&FunctionName);
    }
    if (!Buffer)
    {
        return STATUS_INVALID_PARAMETER;
    }

    MM_COPY_ADDRESS SrcAddr = { 0 };
    SrcAddr.PhysicalAddress.QuadPart = Addr;

    return MmCopyMemory(Buffer, SrcAddr, Size, MM_COPY_MEMORY_PHYSICAL, &BytesTransferred);
}

BOOLEAN GPA2HPA(UINT64 gha, UINT64 eptp, UINT64& hpa)
{
    UINT64 Pml4i = (gha >> PML4I_SHIFT) & 0x1FF;
    UINT64 Pdpti = (gha >> PDPTI_SHIFT) & 0x1FF;
    UINT64 Pdi = (gha >> PDI_SHIFT) & 0x1FF;
    UINT64 Pti = (gha >> PTI_SHIFT) & 0x1FF;

    PML4E Pml4e = { 0 };
    PDPTE Pdpte = { 0 };
    PDE Pde = { 0 };
    PTE Pte = { 0 };

    UINT64 RetBytes = 0;
    eptp &= ~0xFFF;
    hpa = 0;

    NTSTATUS Status = ReadPhysicalMemory(eptp + 8 * Pml4i, &Pml4e, sizeof(Pml4e), RetBytes);
    if (!NT_SUCCESS(Status) || !Pml4e.s.Present)
    {
        MyDbgPrint("Pml4e invalid");
        return FALSE;
    }

    Status = ReadPhysicalMemory(GET_4K_PFN_PAGE(Pml4e.s.PageFrameNumber) + 8 * Pdpti, &Pdpte, sizeof(Pdpte), RetBytes);
    if (!NT_SUCCESS(Status) || !Pdpte.s.Present)
    {
        MyDbgPrint("Pdpte invalid");
        return FALSE;
    }
    if (Pdpte.s.LargePage)
    {
        PDPTE_LARGE* Pdpte_l = (PDPTE_LARGE*)&Pdpte;
        hpa = GET_1G_PFN_PAGE(Pdpte_l->s.PageFrameNumber) + GET_1G_PAGE_OFFSET(gha);
        return TRUE;
    }

    Status = ReadPhysicalMemory(GET_4K_PFN_PAGE(Pdpte.s.PageFrameNumber) + 8 * Pdi, &Pde, sizeof(Pde), RetBytes);
    if (!NT_SUCCESS(Status) || !Pde.s.Present)
    {
        MyDbgPrint("Pde invalid");
        return FALSE;
    }
    if (Pde.s.LargePage)
    {
        PDE_LARGE* Pde_l = (PDE_LARGE*)&Pde;
        hpa = GET_2M_PFN_PAGE(Pde_l->s.PageFrameNumber) + GET_2M_PAGE_OFFSET(gha);
        return TRUE;
    }

    Status = ReadPhysicalMemory(GET_4K_PFN_PAGE(Pde.s.PageFrameNumber) + 8 * Pti, &Pte, sizeof(Pte), RetBytes);
    if (!NT_SUCCESS(Status) || !Pte.s.Present)
    {
        MyDbgPrint("Pte invalid");
        return FALSE;
    }
    hpa = GET_4K_PFN_PAGE(Pte.s.PageFrameNumber) + GET_4K_PAGE_OFFSET(gha);
    return TRUE;
}

NTSTATUS ParsePageTableSelf(
    _In_ UINT64 VA,
    _In_ UINT64 CR3,
    _In_ PVOID Buffer,
    _In_ UINT64 BufferSize,
    _Out_ UINT64& ReadBytes
)
{
    if (!Buffer)
    {
        ReadBytes = 0;
        return STATUS_INVALID_PARAMETER;
    }

    UINT64 Pml4i = (VA >> PML4I_SHIFT) & 0x1FF;
    UINT64 Pdpti = (VA >> PDPTI_SHIFT) & 0x1FF;
    UINT64 Pdi = (VA >> PDI_SHIFT) & 0x1FF;
    UINT64 Pti = (VA >> PTI_SHIFT) & 0x1FF;

    PML4E Pml4e = { 0 };
    PDPTE Pdpte = { 0 };
    PDE Pde = { 0 };
    PTE Pte = { 0 };

    UINT64 RetBytes = 0;
    CR3 &= ~0xFFF;
    NTSTATUS Status = ReadPhysicalMemory(CR3 + 8 * Pml4i, &Pml4e, sizeof(Pml4e), RetBytes);
    if (!NT_SUCCESS(Status) || !Pml4e.s.Present)
    {
        MyDbgPrint("Pml4e invalid");
        return Status;
    }
    MyDbgPrint("Pml4e pfn=%llx", Pml4e.s.PageFrameNumber);

    Status = ReadPhysicalMemory(GET_4K_PFN_PAGE(Pml4e.s.PageFrameNumber) + 8 * Pdpti, &Pdpte, sizeof(Pdpte), RetBytes);
    if (!NT_SUCCESS(Status) || !Pdpte.s.Present)
    {
        MyDbgPrint("Pdpte invalid");
        return Status;
    }
    if (Pdpte.s.LargePage)
    {
        PDPTE_LARGE* Pdpte_l = (PDPTE_LARGE*)&Pdpte;
        MyDbgPrint("Pdpte_l pfn=%llx", Pdpte_l->s.PageFrameNumber);
        return ReadPhysicalMemory(GET_1G_PFN_PAGE(Pdpte_l->s.PageFrameNumber) + GET_1G_PAGE_OFFSET(VA), Buffer, BufferSize, ReadBytes);
    }
    MyDbgPrint("Pdpte pfn=%llx", Pdpte.s.PageFrameNumber);

    Status = ReadPhysicalMemory(GET_4K_PFN_PAGE(Pdpte.s.PageFrameNumber) + 8 * Pdi, &Pde, sizeof(Pde), RetBytes);
    if (!NT_SUCCESS(Status) || !Pde.s.Present)
    {
        MyDbgPrint("Pde invalid");
        return Status;
    }
    if (Pde.s.LargePage)
    {
        PDE_LARGE* Pde_l = (PDE_LARGE*)&Pde;
        MyDbgPrint("Pde_l pfn=%llx", Pde_l->s.PageFrameNumber);
        return ReadPhysicalMemory(GET_2M_PFN_PAGE(Pde_l->s.PageFrameNumber) + GET_2M_PAGE_OFFSET(VA), Buffer, BufferSize, ReadBytes);
    }
    MyDbgPrint("Pde pfn=%llx", Pde.s.PageFrameNumber);

    Status = ReadPhysicalMemory(GET_4K_PFN_PAGE(Pde.s.PageFrameNumber) + 8 * Pti, &Pte, sizeof(Pte), RetBytes);
    if (!NT_SUCCESS(Status) || !Pte.s.Present)
    {
        MyDbgPrint("Pte invalid");
        return Status;
    }
    MyDbgPrint("Pte pfn=%llx", Pte.s.PageFrameNumber);
    return ReadPhysicalMemory(GET_4K_PFN_PAGE(Pte.s.PageFrameNumber) + GET_4K_PAGE_OFFSET(VA), Buffer, BufferSize, ReadBytes);
}


NTSTATUS ReadGPA(
    _In_ UINT64 GPA,
    _In_ UINT64 EPTP,
    _Out_ PVOID Buffer,
    _In_ UINT64 Size,
    _Out_ UINT64& RetBytes
)
{
    RetBytes = 0;
    UINT64 HPA = 0;
    if (!GPA2HPA(GPA, EPTP, HPA))
    {
        return STATUS_UNSUCCESSFUL;
    }
    // MyDbgPrint("HPA=%llx", HPA);
    return ReadPhysicalMemory(HPA, Buffer, Size, RetBytes);
}


BOOL SearchNtoskrnlBaseFromGPA(
    UINT64 GPA,
    UINT64 EPTP,
    SIZE_T Size
)
{
    BOOL Status = FALSE;
    PVOID Buffer = ExAllocatePoolZero(PagedPool, Size, 'SMTG');
    if (Buffer == NULL)
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
        ExFreePoolWithTag(Buffer, 'SMTG');
        Buffer = NULL;
    }
    return Status;
}


NTSTATUS WalkPageTableSelf(
    _In_ UINT64 CR3,
    _In_ UINT64 EPTP,
    _In_ PVOID Buffer,
    _In_ UINT64 BufferSize,
    _Out_ UINT64& ReadBytes
)
{
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(BufferSize);

    UINT64 VA = g_KernelModulesMappingBase;
    UINT64 VA_End = g_KernelModulesMappingEnd;


    UINT64 Pml4i = (VA >> PML4I_SHIFT) & 0x1FF; // 
    UINT64 Pdpti = (VA >> PDPTI_SHIFT) & 0x1FF; // 0
    UINT64 Pdi = (VA >> PDI_SHIFT) & 0x1FF;     // 0
    UINT64 Pti = (VA >> PTI_SHIFT) & 0x1FF;     // 0

    PML4E Pml4[PAGE_ENTRY_NUM] = { 0 };
    PDPTE Pdpt[PAGE_ENTRY_NUM] = { 0 };
    PDE Pd[PAGE_ENTRY_NUM] = { 0 };
    PTE Pt[PAGE_ENTRY_NUM] = { 0 };

    UINT64 RetBytes = 0;
    CR3 &= ~0xFFF;
    NTSTATUS Status = ReadGPA(CR3, EPTP, &Pml4, sizeof(Pml4), RetBytes);
    if (!NT_SUCCESS(Status))
    {
        ReadBytes = 0;
        return Status;
    }

    for (; VA < VA_End && Pml4i < PAGE_ENTRY_NUM; Pml4i++)
    {
        if (!Pml4[Pml4i].s.Present)
        {
            VA += PAGE_SIZE_512G;
            continue;
        }
        // MyDbgPrint("Pml4i=%llx, pfn=%llx", Pml4i, Pml4[Pml4i].s.PageFrameNumber);
        Status = ReadGPA(GET_4K_PFN_PAGE(Pml4[Pml4i].s.PageFrameNumber), EPTP, &Pdpt, sizeof(Pdpt), RetBytes);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
        for (; VA < VA_End && Pdpti < PAGE_ENTRY_NUM; Pdpti++)
        {
            if (!Pdpt[Pdpti].s.Present)
            {
                VA += PAGE_SIZE_1G;
                continue;
            }
            if (Pdpt[Pdpti].s.LargePage)
            {
                PDPTE_LARGE* Pdpte_l = (PDPTE_LARGE*)&Pdpt[Pdpti];
                MyDbgPrint("Pdpte_l pfn=%llx", Pdpte_l->s.PageFrameNumber);
                if (SearchNtoskrnlBaseFromGPA(GET_1G_PFN_PAGE(Pdpte_l->s.PageFrameNumber), EPTP, PAGE_SIZE_1G))
                {
                    MyDbgPrint("Found NT at pdpte_l=%llx, va=%llx", GET_1G_PFN_PAGE(Pdpte_l->s.PageFrameNumber), VA);
                }
                VA += PAGE_SIZE_1G;
            }
            else
            {
                // MyDbgPrint("Pdpti=%llx, pfn=%llx", Pdpti, Pdpt[Pdpti].s.PageFrameNumber);
                Status = ReadGPA(GET_4K_PFN_PAGE(Pdpt[Pdpti].s.PageFrameNumber), EPTP, &Pd, sizeof(Pd), RetBytes);
                if (!NT_SUCCESS(Status))
                {
                    return Status;
                }
                for (; VA < VA_End && Pdi < PAGE_ENTRY_NUM; Pdi++)
                {
                    if (!Pd[Pdi].s.Present)
                    {
                        VA += PAGE_SIZE_2M;
                        continue;
                    }
                    if (Pd[Pdi].s.LargePage)
                    {
                        PDE_LARGE* Pde_l = (PDE_LARGE*)&Pd[Pdi];
                        MyDbgPrint("Pde_l pfn=%llx", Pde_l->s.PageFrameNumber);
                        if (SearchNtoskrnlBaseFromGPA(GET_2M_PFN_PAGE(Pde_l->s.PageFrameNumber), EPTP, PAGE_SIZE_2M))
                        {
                            MyDbgPrint("Found NT at pde_l=%llx, va=%llx", GET_2M_PFN_PAGE(Pde_l->s.PageFrameNumber), VA);
                        }
                        VA += PAGE_SIZE_2M;
                    }
                    else
                    {
                        // MyDbgPrint("Pdi=%llx, pfn=%llx", Pdi, Pd[Pdi].s.PageFrameNumber);
                        Status = ReadGPA(GET_4K_PFN_PAGE(Pd[Pdi].s.PageFrameNumber), EPTP, &Pt, sizeof(Pt), RetBytes);
                        if (!NT_SUCCESS(Status))
                        {
                            return Status;
                        }
                        for (; VA < VA_End && Pti < PAGE_ENTRY_NUM; Pti++)
                        {
                            if (!Pt[Pti].s.Present)
                            {
                                VA += PAGE_SIZE_4K;
                                continue;
                            }
                            // MyDbgPrint("Pte pfn=%llx", Pt[Pti].s.PageFrameNumber);
                            if (SearchNtoskrnlBaseFromGPA(GET_4K_PFN_PAGE(Pt[Pti].s.PageFrameNumber), EPTP, PAGE_SIZE_4K))
                            {
                                MyDbgPrint("Found NT at pte=%llx, va=%llx", GET_4K_PFN_PAGE(Pt[Pti].s.PageFrameNumber), VA);
                            }
                            VA += PAGE_SIZE_4K;
                        }
                        Pti = 0;
                    }
                }
                Pdi = 0;
            }
        }
        Pdpti = 0;
    }
    return Status;
}


