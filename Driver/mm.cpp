#include "global.hpp"
#include "mm.hpp"


NTSTATUS ReadHPA(
    _In_ UINT64 Addr,
    _Out_ PVOID Buffer,
    _In_ UINT64 BufferSize,
    _Out_ UINT64& ReadBytes
)
{
    ReadBytes = 0;
    //if (g_MmCopyMemory == NULL)
    //{
    //    UNICODE_STRING FunctionName;
    //    RtlInitUnicodeString(&FunctionName, L"MmCopyMemory");
    //    g_MmCopyMemory = (fpMmCopyMemory)MmGetSystemRoutineAddress(&FunctionName);
    //}
    if (!Buffer)
    {
        return STATUS_INVALID_PARAMETER_1;
    }
    if (BufferSize > PAGE_SIZE_4K)
    {
        MyDbgPrintEx("BufferSize=%llx", BufferSize);
        return STATUS_INVALID_PARAMETER_2;
    }

    MM_COPY_ADDRESS SrcAddr = { 0 };
    SrcAddr.PhysicalAddress.QuadPart = Addr;

    return MmCopyMemory(Buffer, SrcAddr, BufferSize, MM_COPY_MEMORY_PHYSICAL, &ReadBytes);
}

BOOLEAN GPA2HPA(
    _In_ UINT64 GPA,
    _In_ UINT64 EPTP,
    _Out_ UINT64& HPA
)
{
    UINT64 Pml4i = (GPA >> PML4I_SHIFT) & 0x1FF;
    UINT64 Pdpti = (GPA >> PDPTI_SHIFT) & 0x1FF;
    UINT64 Pdi = (GPA >> PDI_SHIFT) & 0x1FF;
    UINT64 Pti = (GPA >> PTI_SHIFT) & 0x1FF;

    PML4E Pml4e = { 0 };
    PDPTE Pdpte = { 0 };
    PDE Pde = { 0 };
    PTE Pte = { 0 };

    UINT64 RetBytes = 0;
    EPTP &= ~0xFFF;
    HPA = 0;

    NTSTATUS Status = ReadHPA(EPTP + 8 * Pml4i, &Pml4e, sizeof(Pml4e), RetBytes);
    if (!NT_SUCCESS(Status) || !Pml4e.s.Present)
    {
        MyDbgPrintEx("Pml4e=%llx is invalid, HPA=%llx", Pml4e.AsUInt, EPTP + 8 * Pml4i);
        return FALSE;
    }

    Status = ReadHPA(GET_4K_PFN_PAGE(Pml4e.s.PageFrameNumber) + 8 * Pdpti, &Pdpte, sizeof(Pdpte), RetBytes);
    if (!NT_SUCCESS(Status) || !Pdpte.s.Present)
    {
        MyDbgPrintEx("Pdpte=%llx is invalid, HPA=%llx", Pdpte.AsUInt, GET_4K_PFN_PAGE(Pml4e.s.PageFrameNumber) + 8 * Pdpti);
        return FALSE;
    }
    if (Pdpte.s.LargePage)
    {
        PDPTE_LARGE* Pdpte_l = (PDPTE_LARGE*)&Pdpte;
        HPA = GET_1G_PFN_PAGE(Pdpte_l->s.PageFrameNumber) + GET_1G_PAGE_OFFSET(GPA);
        return TRUE;
    }

    Status = ReadHPA(GET_4K_PFN_PAGE(Pdpte.s.PageFrameNumber) + 8 * Pdi, &Pde, sizeof(Pde), RetBytes);
    if (!NT_SUCCESS(Status) || !Pde.s.Present)
    {
        MyDbgPrintEx("Pde=%llx is invalid, HPA=%llx", Pde.AsUInt, GET_4K_PFN_PAGE(Pdpte.s.PageFrameNumber) + 8 * Pdi);
        return FALSE;
    }
    if (Pde.s.LargePage)
    {
        PDE_LARGE* Pde_l = (PDE_LARGE*)&Pde;
        HPA = GET_2M_PFN_PAGE(Pde_l->s.PageFrameNumber) + GET_2M_PAGE_OFFSET(GPA);
        return TRUE;
    }

    Status = ReadHPA(GET_4K_PFN_PAGE(Pde.s.PageFrameNumber) + 8 * Pti, &Pte, sizeof(Pte), RetBytes);
    if (!NT_SUCCESS(Status) || !Pte.s.Present)
    {
        MyDbgPrintEx("Pte=%llx is invalid, HPA=%llx", Pte.AsUInt, GET_4K_PFN_PAGE(Pde.s.PageFrameNumber) + 8 * Pti);
        return FALSE;
    }
    HPA = GET_4K_PFN_PAGE(Pte.s.PageFrameNumber) + GET_4K_PAGE_OFFSET(GPA);
    return TRUE;
}

NTSTATUS ReadGPA(
    _In_ UINT64 GPA,
    _In_ UINT64 EPTP,
    _Out_ PVOID Buffer,
    _In_ UINT64 BufferSize,
    _Out_ UINT64& ReadBytes
)
{
    ReadBytes = 0;
    UINT64 HPA = 0;
    if (!GPA2HPA(GPA, EPTP, HPA))
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (!Buffer)
    {
        return STATUS_INVALID_PARAMETER;
    }
    return ReadHPA(HPA, Buffer, BufferSize, ReadBytes);
}

BOOLEAN GVA2GPA(
    _In_ UINT64 GVA,
    _In_ UINT64 CR3,
    _In_ UINT64 EPTP,
    _Out_ UINT64& GPA
)
{
    UINT64 Pml4i = (GVA >> PML4I_SHIFT) & 0x1FF;
    UINT64 Pdpti = (GVA >> PDPTI_SHIFT) & 0x1FF;
    UINT64 Pdi = (GVA >> PDI_SHIFT) & 0x1FF;
    UINT64 Pti = (GVA >> PTI_SHIFT) & 0x1FF;

    PML4E Pml4e = { 0 };
    PDPTE Pdpte = { 0 };
    PDE Pde = { 0 };
    PTE Pte = { 0 };

    UINT64 RetBytes = 0;
    CR3 &= ~0xFFF;
    EPTP &= ~0xFFF;
    GPA = 0;

    NTSTATUS Status = ReadGPA(CR3 + 8 * Pml4i, EPTP, &Pml4e, sizeof(Pml4e), RetBytes);
    if (!NT_SUCCESS(Status) || !Pml4e.s.Present)
    {
        MyDbgPrintEx("Pml4e=%llx is invalid, GPA=%llx", Pml4e.AsUInt, CR3 + 8 * Pml4i);
        return FALSE;
    }

    Status = ReadGPA(GET_4K_PFN_PAGE(Pml4e.s.PageFrameNumber) + 8 * Pdpti, EPTP, &Pdpte, sizeof(Pdpte), RetBytes);
    if (!NT_SUCCESS(Status) || !Pdpte.s.Present)
    {
        MyDbgPrintEx("Pdpte=%llx is invalid, GPA=%llx", Pdpte.AsUInt, GET_4K_PFN_PAGE(Pml4e.s.PageFrameNumber) + 8 * Pdpti);
        return FALSE;
    }
    if (Pdpte.s.LargePage)
    {
        PDPTE_LARGE* Pdpte_l = (PDPTE_LARGE*)&Pdpte;
        GPA = GET_1G_PFN_PAGE(Pdpte_l->s.PageFrameNumber) + GET_1G_PAGE_OFFSET(GVA);
        return TRUE;
    }

    Status = ReadGPA(GET_4K_PFN_PAGE(Pdpte.s.PageFrameNumber) + 8 * Pdi, EPTP, &Pde, sizeof(Pde), RetBytes);
    if (!NT_SUCCESS(Status) || !Pde.s.Present)
    {
        MyDbgPrintEx("Pde=%llx is invalid, GPA=%llx", Pde.AsUInt, GET_4K_PFN_PAGE(Pdpte.s.PageFrameNumber) + 8 * Pdi);
        return FALSE;
    }
    if (Pde.s.LargePage)
    {
        PDE_LARGE* Pde_l = (PDE_LARGE*)&Pde;
        GPA = GET_2M_PFN_PAGE(Pde_l->s.PageFrameNumber) + GET_2M_PAGE_OFFSET(GVA);
        return TRUE;
    }

    Status = ReadGPA(GET_4K_PFN_PAGE(Pde.s.PageFrameNumber) + 8 * Pti, EPTP, &Pte, sizeof(Pte), RetBytes);
    if (!NT_SUCCESS(Status) || !Pte.s.Present)
    {
        MyDbgPrintEx("Pte=%llx is invalid, GPA=%llx", Pte.AsUInt, GET_4K_PFN_PAGE(Pde.s.PageFrameNumber) + 8 * Pti);
        return FALSE;
    }
    GPA = GET_4K_PFN_PAGE(Pte.s.PageFrameNumber) + GET_4K_PAGE_OFFSET(GVA);
    return TRUE;
}

NTSTATUS ReadGVA(
    _In_ UINT64 GVA,
    _In_ UINT64 CR3,
    _In_ UINT64 EPTP,
    _Out_ PVOID Buffer,
    _In_ UINT64 BufferSize,
    _Out_ UINT64& ReadBytes
)
{
    ReadBytes = 0;
    UINT64 GPA = 0;
    if (!GVA2GPA(GVA, CR3, EPTP, GPA))
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (!Buffer)
    {
        return STATUS_INVALID_PARAMETER;
    }
    return ReadGPA(GPA, EPTP, Buffer, BufferSize, ReadBytes);
}

NTSTATUS GetNtosBaseByEnumPageTable(
    _In_ UINT64 CR3,
    _In_ UINT64 EPTP,
    _Out_ UINT64& NtoskrnlGPA,
    _Out_ UINT64& NtoskrnlGVA
)
{
    UINT64 VA = g_KernelModulesMappingBase;
    UINT64 VA_End = g_KernelModulesMappingEnd;

    UINT64 Pml4i = (VA >> PML4I_SHIFT) & 0x1FF;
    UINT64 Pdpti = (VA >> PDPTI_SHIFT) & 0x1FF;
    UINT64 Pdi = (VA >> PDI_SHIFT) & 0x1FF;
    UINT64 Pti = (VA >> PTI_SHIFT) & 0x1FF;

    PML4E Pml4[PAGE_ENTRY_NUM] = { 0 };
    PDPTE Pdpt[PAGE_ENTRY_NUM] = { 0 };
    PDE Pd[PAGE_ENTRY_NUM] = { 0 };
    PTE Pt[PAGE_ENTRY_NUM] = { 0 };

    CR3 &= ~0xFFF;
    NtoskrnlGPA = 0;
    NtoskrnlGVA = 0;
    UINT64 ReadBytes = 0;
    NTSTATUS Status = ReadGPA(CR3, EPTP, &Pml4, sizeof(Pml4), ReadBytes);
    if (!NT_SUCCESS(Status))
    {
        MyDbgPrintEx("Pml4=%llx is invalid", CR3);
        return Status;
    }
    for (; VA < VA_End && Pml4i < PAGE_ENTRY_NUM; Pml4i++)
    {
        if (!Pml4[Pml4i].s.Present)
        {
            VA += PAGE_SIZE_512G;
            Pdpti = Pdi = Pti = 0;
            continue;
        }
        Status = ReadGPA(GET_4K_PFN_PAGE(Pml4[Pml4i].s.PageFrameNumber), EPTP, &Pdpt, sizeof(Pdpt), ReadBytes);
        if (!NT_SUCCESS(Status))
        {
            MyDbgPrintEx("Pdpt=%llx is invalid", GET_4K_PFN_PAGE(Pml4[Pml4i].s.PageFrameNumber));
            return Status;
        }
        for (; VA < VA_End && Pdpti < PAGE_ENTRY_NUM; Pdpti++)
        {
            if (!Pdpt[Pdpti].s.Present)
            {
                VA += PAGE_SIZE_1G;
                Pdi = Pti = 0;
                continue;
            }
            if (!Pdpt[Pdpti].s.LargePage)
            {
                Status = ReadGPA(GET_4K_PFN_PAGE(Pdpt[Pdpti].s.PageFrameNumber), EPTP, &Pd, sizeof(Pd), ReadBytes);
                if (!NT_SUCCESS(Status))
                {
                    MyDbgPrintEx("Pdt=%llx is invalid", GET_4K_PFN_PAGE(Pdpt[Pdpti].s.PageFrameNumber));
                    return Status;
                }
                for (; VA < VA_End && Pdi < PAGE_ENTRY_NUM; Pdi++)
                {
                    if (!Pd[Pdi].s.Present)
                    {
                        VA += PAGE_SIZE_2M;
                        Pti = 0;
                        continue;
                    }
                    if (!Pd[Pdi].s.LargePage)
                    {
                        Status = ReadGPA(GET_4K_PFN_PAGE(Pd[Pdi].s.PageFrameNumber), EPTP, &Pt, sizeof(Pt), ReadBytes);
                        if (!NT_SUCCESS(Status))
                        {
                            MyDbgPrintEx("Pt=%llx is invalid", GET_4K_PFN_PAGE(Pd[Pdi].s.PageFrameNumber));
                            return Status;
                        }
                        for (; VA < VA_End && Pti < PAGE_ENTRY_NUM; Pti++)
                        {
                            if (!Pt[Pti].s.Present)
                            {
                                VA += PAGE_SIZE_4K;
                                continue;
                            }
                            // read
                            if (IsGPANtoskrnlBase(GET_4K_PFN_PAGE(Pt[Pti].s.PageFrameNumber), EPTP, PAGE_SIZE_4K))
                            {
                                NtoskrnlGPA = GET_4K_PFN_PAGE(Pt[Pti].s.PageFrameNumber);
                                NtoskrnlGVA = VA;
                                //MyDbgPrintEx("gpa=%llx, va=%llx", NtoskrnlGPA, VA);
                            }
                            VA += PAGE_SIZE_4K;
                        }
                    }
                    else
                    {
                        // read
                        PDE_LARGE* Pde_l = (PDE_LARGE*)&Pd[Pdi];
                        if (IsGPANtoskrnlBase(GET_2M_PFN_PAGE(Pde_l->s.PageFrameNumber), EPTP, PAGE_SIZE_2M))
                        {
                            NtoskrnlGPA = GET_2M_PFN_PAGE(Pde_l->s.PageFrameNumber);
                            NtoskrnlGVA = VA;
                            //MyDbgPrintEx("gpa=%llx, va=%llx", NtoskrnlGPA, VA);
                        }
                        VA += PAGE_SIZE_2M;
                    }
                    Pti = 0;
                }
            }
            else
            {
                // read
                PDPTE_LARGE* Pdpte_l = (PDPTE_LARGE*)&Pdpt[Pdpti];
                if (IsGPANtoskrnlBase(GET_1G_PFN_PAGE(Pdpte_l->s.PageFrameNumber), EPTP, PAGE_SIZE_1G))
                {
                    NtoskrnlGPA = GET_1G_PFN_PAGE(Pdpte_l->s.PageFrameNumber);
                    NtoskrnlGVA = VA;
                    //MyDbgPrintEx("gpa=%llx, va=%llx", NtoskrnlGPA, VA);
                }
                VA += PAGE_SIZE_1G; 
            }
            Pdi = Pti = 0;
        }
        Pdpti = Pdi = Pti = 0;
    }
    return Status;
}


NTSTATUS PrintGuestProcessList(
    _In_ UINT64 NtoskrnlGVA,
    _In_ WinRelatedData& Offset,
    _In_ UINT64 CR3,
    _In_ UINT64 EPTP
)
{
    UINT64 ReadBytes = 0;
    UINT64 PsActiveProcessHead = 0;
    NTSTATUS Status = ReadGVA(NtoskrnlGVA + Offset.PsActiveProcessHead_Ntoskrnl, CR3, EPTP, &PsActiveProcessHead, sizeof(PsActiveProcessHead), ReadBytes);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    UINT64 VA = PsActiveProcessHead;
    LIST_ENTRY head = { 0 };
    do
    {
        PEPROCESS Process = PEPROCESS(VA - Offset.ActiveProcessLinks_EPROCESS);
        UINT64 Proc_pid = 0;
        UINT64 Proc_cr3 = 0;
        CHAR Proc_name[16] = { 0 };

        Status = ReadGVA((UINT64)((PCHAR)Process + Offset.UniqueProcessId_EPROCESS), CR3, EPTP, &Proc_pid, sizeof(Proc_pid), ReadBytes);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
        Status = ReadGVA((UINT64)((PCHAR)Process + Offset.DirectoryTableBase_KPROCESS), CR3, EPTP, &Proc_cr3, sizeof(Proc_cr3), ReadBytes);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
        Status = ReadGVA((UINT64)((PCHAR)Process + Offset.ImageFileName_EPROCESS), CR3, EPTP, &Proc_name, sizeof(Proc_name) - 1, ReadBytes);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        //MyDbgPrintEx("VA=%p, Process=%p", VA, Process);
        MyDbgPrintEx("pid=%llx, cr3=%llx, name=%s", Proc_pid, Proc_cr3, Proc_name);

        Status = ReadGVA(VA, CR3, EPTP, &head, sizeof(head), ReadBytes);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
        //MyDbgPrintEx("Flink=%p Blink=%p", head.Flink, head.Blink);
        VA = (UINT64)head.Flink;
    } while (VA != PsActiveProcessHead && VA != NtoskrnlGVA + Offset.PsActiveProcessHead_Ntoskrnl);
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

    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    ReadBytes = 0;
    UINT64 NtoskrnlGPA = 0;
    UINT64 NtoskrnlGVA = 0;
    if (!NT_SUCCESS(GetNtosBaseByEnumPageTable(CR3, EPTP, NtoskrnlGPA, NtoskrnlGVA)))
    {
        return Status;
    }
    MyDbgPrintEx("NtoskrnlGPA=%llx, NtoskrnlGVA=%llx", NtoskrnlGPA, NtoskrnlGVA);

    WinRelatedData Offset = g_offset_1903_18363_Nt18362;
    if (!NT_SUCCESS(PrintGuestProcessList(NtoskrnlGVA, Offset, CR3, EPTP)))
    {
        return Status;
    }
    return Status;
}

