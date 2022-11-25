//
// Created by mirasrael on 11/25/2022.
//

#include "FarJumpWriter.h"

void FarJumpWriter::WriteTo(unsigned char *dst) const {
    movWriter.WriteTo(dst);
    dst += movWriter.GetDataSize();
    // jmp rax
    dst[0] = 0xff;
    dst[1] = 0xe0;
}
