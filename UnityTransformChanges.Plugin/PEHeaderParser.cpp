//
// Created by mirasrael on 11/25/2022.
//

#include "PEHeaderParser.h"

PCODE_VIEW_DEBUG_ENTRY PEHeaderParser::GetCodeViewDebugEntry(HMODULE handle) {
    auto dosHeader = (PIMAGE_DOS_HEADER)handle;
    auto base = (uint64_t)handle;
    auto imageNTHeaders = (PIMAGE_NT_HEADERS)(base + dosHeader->e_lfanew);
    auto debugEntry = imageNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    auto debugDirectoriesCount = debugEntry.Size / sizeof(IMAGE_DEBUG_DIRECTORY);
    for (auto i = 0; i < debugDirectoriesCount; ++i)
    {
        auto debugDirectory = (PIMAGE_DEBUG_DIRECTORY)(base + debugEntry.VirtualAddress) + i;
        if (debugDirectory->Type == IMAGE_DEBUG_TYPE_CODEVIEW)
            return (PCODE_VIEW_DEBUG_ENTRY) (base + debugDirectory->AddressOfRawData);

    }
    return nullptr;
}
