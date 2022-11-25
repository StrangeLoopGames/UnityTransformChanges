//
// Created by mirasrael on 11/25/2022.
//

#ifndef UNITYTRANSFORMCHANGES_PATCHSEQUENCE_H
#define UNITYTRANSFORMCHANGES_PATCHSEQUENCE_H

#include <vector>
#include "Writers/MemoryWriter.h"

struct PatchSequence : MemoryWriter
{
public:
    const std::vector<unsigned char> bytesToReplace;

    PatchSequence(std::initializer_list<unsigned char> bytesToReplace) : bytesToReplace(std::vector<unsigned char>(bytesToReplace)) { }

    unsigned long long int GetDataSize() const override
    {
        return bytesToReplace.size();
    }

    void WriteTo(unsigned char *dst) const override;
};

#endif //UNITYTRANSFORMCHANGES_PATCHSEQUENCE_H
