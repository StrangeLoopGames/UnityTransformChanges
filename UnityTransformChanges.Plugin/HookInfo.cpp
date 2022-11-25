//
// Created by mirasrael on 11/25/2022.
//

#include "PatchSequence.h"
#include "Writers/FarJumpWriter.h"
#include "Writers/MovToRaxWriter.h"
#include "Writers/MemoryWriter.h"
#include "HookInfo.h"

void HookInfo::Init(const PatchSequence &patchSequence, void *returnAddress) {
    FarJumpWriter farJumpWriter(returnAddress);
    patchSequence.WriteTo(ResumeOpCodes);
    farJumpWriter.WriteTo(ResumeOpCodes + patchSequence.GetDataSize());
    // Mark this code as executable, it located in data section and need explicit permissions
    unsigned long oldProtect;
    VirtualProtect(ResumeOpCodes, patchSequence.GetDataSize() + farJumpWriter.GetDataSize(), PAGE_EXECUTE_READWRITE, &oldProtect);

    unsigned char * dst = HookInvokeOpCodes;
    MovToRaxWriter((unsigned long long int)this).WriteTo(dst);
    dst += MovToRaxWriter::OpCodesLength;
    // jmp [rax + offsetof(InterceptorHookAddress)]
    *dst++ = 0xFF;
    *dst++ = 0x60;
    *dst   = sizeof(Hook) + sizeof(ResumeOpCodes);
    VirtualProtect(HookInvokeOpCodes, sizeof(HookInvokeOpCodes), PAGE_EXECUTE_READWRITE, &oldProtect);
}
