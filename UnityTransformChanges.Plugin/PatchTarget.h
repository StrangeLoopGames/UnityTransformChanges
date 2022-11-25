//
// Created by mirasrael on 11/25/2022.
//

#ifndef UNITYTRANSFORMCHANGES_PATCHTARGET_H
#define UNITYTRANSFORMCHANGES_PATCHTARGET_H

#include "PatchSequence.h"

struct PatchTarget
{
    const unsigned long long int Rva;
    PatchSequence PatchSequence;
    const void* Hook;

    PatchTarget(unsigned long long int rva, struct PatchSequence patchSequence, void* hook = nullptr) : Rva(rva), PatchSequence(std::move(patchSequence)), Hook(hook) { }
};

#endif //UNITYTRANSFORMCHANGES_PATCHTARGET_H
