//
// Created by mirasrael on 11/25/2022.
//

#include "pch.h"
#include "PatchSequence.h"
#include "Writers/FarJumpWriter.h"
#include "Writers/MemoryWriter.h"
#include "Patch.h"
#include "Debug.h"

/*
 * We use that fact what functions aligned for 0x10 and gaps filled with 0xcc (int 3) op codes.
 * We can try to find a big enough gap (size <= 15)
 */
void *FindGap(void *start, unsigned long long int size) {
    auto aligned = (unsigned char *)start;

    auto address = aligned - 1;
    auto gapStart = aligned - size;
    do {
        if (*(unsigned char *) address != 0xcc)
        {
            // wrong instruction, try next aligned section
            aligned += 0x10;
            address = aligned - 1;
            gapStart = aligned - size;
        }
        else
            --address;
    } while (address >= gapStart);
    return gapStart;
}

unsigned static long long int GetPatchAddress(const PatchTarget& target)
{
    return (unsigned long long int)CurrentUnityRuntime.RvaBase + target.Rva;
}

inline static const PatchTarget& SelectTarget(const PatchTarget& editor, const PatchTarget& mono, const PatchTarget& monoDevelopment, const PatchTarget& il2cpp, const PatchTarget& il2cppDevelopment)
{
    switch (CurrentUnityRuntime.Type) {
        case Editor:
            return editor;
        case Mono:
            return mono;
        case MonoDevelopment:
            return monoDevelopment;
        case IL2CPP:
            return il2cpp;
        case IL2CPPDevelopment:
            return il2cppDevelopment;
        default:
            throw std::exception();
    }
}

bool Patch::Apply() {
    // DebugBreakIfAttached();
    auto address = GetPatchAddress(target);
    const PatchSequence& patchSequence = target.PatchSequence;
    auto cmp = memcmp((void*)address, patchSequence.bytesToReplace.data(), patchSequence.GetDataSize());
    if (cmp != 0)
    {
        MessageBox(nullptr, "Failed to apply patch, because bytes to replace doesn't match", "Error", MB_OK);
        DebugBreakIfAttached();
        return false;
    }

    if (farJumpAddress == nullptr)
        farJumpAddress = FindGap((void*)address, FarJumpWriter::OpCodesLength);

    // jmp far_jump
    unsigned long oldProtect;
    VirtualProtect((void*) address, patchSequence.GetDataSize(), PAGE_EXECUTE_READWRITE, &oldProtect);
    unsigned char patch[patchSequence.GetDataSize()];
    memset(patch, 0xcc, patchSequence.GetDataSize());
    auto offset = (long long int)((unsigned long long int)farJumpAddress - address - 2); // jmp byte offset has 2 bytes size
    if (offset >= -128 && offset <= 127)
    {
        patch[0] = 0xeb;
        patch[1] = offset;
    }
    else
    {
        patch[0] = 0xe9;
        *(unsigned int *)(patch + 1) = offset - 3;                       // extra 3 bytes for 5 byte size jmp with 32-bit offset
    }
    memcpy((void*)address, patch, patchSequence.GetDataSize());
    VirtualProtect((void*) address, patchSequence.GetDataSize(), oldProtect, &oldProtect);

    void *resumeAt = (void *) (address + patchSequence.GetDataSize());
    hookInfo.Init(patchSequence, resumeAt);
    hookInfo.HookWriter().WriteProtectedTo((unsigned char *)farJumpAddress);
    return true;
}

void Patch::Rollback() {
    auto address = GetPatchAddress(target);
    target.PatchSequence.WriteProtectedTo((unsigned char *)address);
}

Patch::Patch(const PatchTarget &editor, const PatchTarget& mono, const PatchTarget& monoDevelopment, const PatchTarget& il2cpp, const PatchTarget& il2cppDevelopment, const void *defaultHook) : Patch(SelectTarget(editor, mono, monoDevelopment, il2cpp, il2cppDevelopment), defaultHook) { }

Patch::Patch(const PatchTarget &target, const void *defaultHook) : target(target), hookInfo(target.Hook != nullptr ? target.Hook : defaultHook), farJumpAddress(nullptr) { }
