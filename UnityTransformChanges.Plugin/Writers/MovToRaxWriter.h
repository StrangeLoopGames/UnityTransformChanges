//
// Created by mirasrael on 11/25/2022.
//

#ifndef UNITYTRANSFORMCHANGES_MOVTORAXWRITER_H
#define UNITYTRANSFORMCHANGES_MOVTORAXWRITER_H

#include "MemoryWriter.h"

struct MovToRaxWriter : MemoryWriter
{
public:
    static const int OpCodesLength = 10;

    explicit MovToRaxWriter(const unsigned long long int value) : Value(value) { }

    const unsigned long long int Value;

    inline unsigned long long int GetDataSize() const override
    {
        return OpCodesLength;
    }

    void WriteTo(unsigned char * dst) const override;
};

#endif //UNITYTRANSFORMCHANGES_MOVTORAXWRITER_H
