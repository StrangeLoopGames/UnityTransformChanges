//
// Created by mirasrael on 11/25/2022.
//

#ifndef UNITYTRANSFORMCHANGES_PEHEADERPARSER_H
#define UNITYTRANSFORMCHANGES_PEHEADERPARSER_H

#include "pch.h"

typedef struct
{
    DWORD  CodeViewSignature;
    GUID   Signature;
    DWORD  Age;
    const char   PdbFileNameFirstChar;
    LPCSTR GetPdbFileName() const { return &PdbFileNameFirstChar; }
} CODE_VIEW_DEBUG_ENTRY, *PCODE_VIEW_DEBUG_ENTRY;

class PEHeaderParser {
public:
    static PCODE_VIEW_DEBUG_ENTRY GetCodeViewDebugEntry(HMODULE handle);
};


#endif //UNITYTRANSFORMCHANGES_PEHEADERPARSER_H
