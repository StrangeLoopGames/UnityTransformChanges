//
// Created by mirasrael on 11/25/2022.
//

#ifndef UNITYTRANSFORMCHANGES_PATCH_H
#define UNITYTRANSFORMCHANGES_PATCH_H

#include "pch.h"
#include "HookInfo.h"
#include "UnityRuntime.h"
#include "Writers/FarJumpWriter.h"
#include "PatchTarget.h"

class Patch
{
    void* farJumpAddress;
    HookInfo hookInfo;
    PatchTarget target;
public:
    Patch(const PatchTarget& target, const void* defaultHook);

    Patch(const PatchTarget& editor, const PatchTarget& mono, const PatchTarget& il2cpp, const void* defaultHook);

    bool Apply();

    void Rollback();
};

#endif //UNITYTRANSFORMCHANGES_PATCH_H
