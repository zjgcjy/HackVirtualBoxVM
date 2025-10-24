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
#ifdef __cplusplus
}
#endif // __cplusplus

BOOL AobSearcher(
    _Out_ PCHAR Buffer,
    _In_ SIZE_T BufferSize,
    _In_ const PCHAR Pattern,
    _In_ const PCHAR Mask,
    _Out_ SIZE_T& Offset
);
