//
// Created by mirasrael on 11/25/2022.
//

#include <vector>
#include "PatchSequence.h"

void PatchSequence::WriteTo(unsigned char *dst) const {
    std::copy(bytesToReplace.begin(), bytesToReplace.end(), dst);
}
