//
// Created by mirasrael on 11/25/2022.
//

#include "MemoryWriter.h"

void MemoryWriter::WriteProtectedTo(unsigned char *dst) const {
    auto dataSize = GetDataSize();
    unsigned long oldProtect;
    VirtualProtect(dst, dataSize, PAGE_EXECUTE_READWRITE, &oldProtect);
    this->WriteTo(dst);
    VirtualProtect(dst, dataSize, oldProtect, &oldProtect);
}
