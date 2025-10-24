#include "aob.hpp"

BOOL AobSearcher(
    _In_ PCHAR Buffer,
    _In_ SIZE_T BufferSize,
    _In_ const PCHAR Pattern,
    _In_ const PCHAR Mask,
    _Out_ SIZE_T& Offset)
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
