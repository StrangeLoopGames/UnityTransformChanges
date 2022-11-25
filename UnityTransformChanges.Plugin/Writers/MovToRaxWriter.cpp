//
// Created by mirasrael on 11/25/2022.
//

#include "MovToRaxWriter.h"

void MovToRaxWriter::WriteTo(unsigned char *dst) const {
    // movabs rax, Value
    dst[0] = 0x48;
    dst[1] = 0xb8;
    *(unsigned long long int *)&dst[2] = Value;
}
