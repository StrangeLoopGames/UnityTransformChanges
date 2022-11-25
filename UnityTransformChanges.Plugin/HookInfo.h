//
// Created by mirasrael on 11/25/2022.
//

#ifndef UNITYTRANSFORMCHANGES_HOOKINFO_H
#define UNITYTRANSFORMCHANGES_HOOKINFO_H

#include "PatchSequence.h"
#include "Writers/MovToRaxWriter.h"
#include "Writers/FarJumpWriter.h"

#define INTERCEPTOR_HOOK_START(ret, methodName, ...) ret methodName(__VA_ARGS__) { \
    register ret (*_rax)(__VA_ARGS__) asm("%rax"); \
    auto originalFunction = _rax;
#define INTERCEPTOR_HOOK_END() }

class HookInfo
{
    static __declspec(naked) void InterceptorHook()
    {
        asm volatile(R"(
            movq        %rax, 0x08(%rsp)  # save HookInfo reference
            movq        (%rax), %rax      # move Hook address to rax
            movq        %rax, 0x10(%rsp)  # save Hook address
            movq        0x08(%rsp), %rax  # restore HookInfo reference
            addq        $0x08, %rax       # calculate address of ResumeOpCodes and save to eax
            jmp         *0x10(%rsp)       # jump to Hook
        )");
    }

    const void* Hook;
    unsigned char ResumeOpCodes[32];
    const void* InterceptorHookAddress = (void*)InterceptorHook; // need this for jmp in HookInvokeOpCodes using [rax + offset InterceptorHookAddress]
    unsigned char HookInvokeOpCodes[MovToRaxWriter::OpCodesLength + 3]; // mov + jmp [rax + InterceptorHookAddress]
public:
    explicit HookInfo(const void* hook) : Hook(hook), ResumeOpCodes(), HookInvokeOpCodes() { }

    void Init(const PatchSequence& patchSequence, void* returnAddress);

    FarJumpWriter HookWriter() { return FarJumpWriter(HookInvokeOpCodes); }
};

#endif //UNITYTRANSFORMCHANGES_HOOKINFO_H
