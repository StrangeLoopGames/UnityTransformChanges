//
// Created by mirasrael on 11/25/2022.
//

#ifndef UNITYTRANSFORMCHANGES_FARJUMPWRITER_H
#define UNITYTRANSFORMCHANGES_FARJUMPWRITER_H

#include "MemoryWriter.h"
#include "MovToRaxWriter.h"

struct FarJumpWriter : MemoryWriter
{
    MovToRaxWriter movWriter;
public:
    static const int OpCodesLength = MovToRaxWriter::OpCodesLength + 2;

    explicit FarJumpWriter(const void* targetAddress) : movWriter((unsigned long long int)targetAddress) { }

    inline unsigned long long int GetDataSize() const override
    {
        return OpCodesLength;
    }

    void WriteTo(unsigned char * dst) const override;
};

#endif //UNITYTRANSFORMCHANGES_FARJUMPWRITER_H
