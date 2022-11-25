//
// Created by mirasrael on 11/25/2022.
//

#ifndef UNITYTRANSFORMCHANGES_MEMORYWRITER_H
#define UNITYTRANSFORMCHANGES_MEMORYWRITER_H

#include <memoryapi.h>

struct MemoryWriter
{
    virtual unsigned long long int GetDataSize() const = 0;

    virtual void WriteTo(unsigned char * dst) const = 0;

    void WriteProtectedTo(unsigned char * dst) const;
};

#endif //UNITYTRANSFORMCHANGES_MEMORYWRITER_H
