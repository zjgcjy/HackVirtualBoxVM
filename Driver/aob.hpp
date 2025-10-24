#pragma once

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
#include <fltKernel.h>
#ifdef __cplusplus
}
#endif // __cplusplus

#include "mm.hpp"


BOOL AobSearcher(PCHAR Buffer, SIZE_T BufferSize, const PCHAR Pattern, const PCHAR Mask, SIZE_T &Offset)
{
    /*if (strlen(Pattern) != strlen(Mask))
    {
        MyDbgPrint("AobSearcher1 failed");
        return FALSE;
    }*/
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

