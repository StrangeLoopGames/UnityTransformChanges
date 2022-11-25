//
// Created by mirasrael on 11/25/2022.
//

#include "Debug.h"
#include "MonoAssembly.h"

void* SearchByPattern(void* address, uint64_t pattern, uint64_t mask, uint32_t maxDistance)
{
    auto ptr = (uint64_t)address;
    auto end = ptr + maxDistance;
    for (; ptr < end; ++ptr)
    {
        if (((*(uint64_t*)ptr) & mask) == pattern)
            return (void*)ptr;
    }
    return nullptr;
}

void* SearchAndResolveByRelativeCall(void* caller, uint64_t offset)
{
    auto callAddress = SearchByPattern(caller, 0xe8 | (offset << 8),  0xffffffffff, 1024);
    return callAddress != nullptr ? (void*)((uint64_t) callAddress + offset + 5) : nullptr;
}

void DebugTransforms()
{
    auto assembly = (MonoAssembly*) malloc(sizeof(MonoAssembly));
    auto inited = assembly->Init("D:/projects/testing/UnityTest/Build VS/build/bin/UnityTest_Data/Managed/UnityEngine.CoreModule.dll");
    if (inited)
    {
        auto monoFuncAddress = assembly->LookupInternalMethod("UnityEngine.Transform:set_position_Injected");

        if (monoFuncAddress != nullptr)
        {
            auto rvaBasePlayer = GetModuleHandle("UnityPlayer.dll");
            auto rvaBaseEditor = GetModuleHandle("Unity.exe");

            HANDLE rvaBase;
            void* funcAddress;
            if ((rvaBase = rvaBasePlayer) != 0)
                funcAddress = SearchAndResolveByRelativeCall(monoFuncAddress, 0x00aaf45e);
            else if ((rvaBase = rvaBaseEditor) != 0)
                funcAddress = SearchAndResolveByRelativeCall(monoFuncAddress, 0x00c62ba1);
            else
                MessageBox(nullptr, "Can't resolve module", "Test", MB_OK);

            if (funcAddress != nullptr) {
                char message[1000];
                message[0] = 0;
                sprintf_s(message, sizeof(message), "set_Position VA: 0x%x RVA: 0x%x RVABase: 0x%x",
                          (uint64_t) funcAddress,
                          (uint64_t) funcAddress - (uint64_t) rvaBase, rvaBase);
                MessageBox(nullptr, message, "Test", MB_OK);
            } else
                MessageBox(nullptr, "Can't find call by pattern", "Test", MB_OK);
        }
        else
            MessageBoxA(nullptr, "No Func", "Test", MB_OK);
    }
    else
        MessageBox(nullptr, "Failed to Init Mono", "Test", MB_OK);

    assembly->Dispose();
    free(assembly);
}

void DebugBreakIfAttached()
{
    if (IsDebuggerPresent())
        DebugBreak();
}