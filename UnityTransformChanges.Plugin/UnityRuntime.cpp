//
// Created by mirasrael on 11/25/2022.
//

#include "pch.h"
#include "UnityRuntime.h"
#include "PEHeaderParser.h"

UnityRuntime CurrentUnityRuntime;

UnityRuntime::UnityRuntime()
{
    HMODULE module = GetModuleHandle("UnityPlayer.dll");
    if (module != nullptr)
    {
        auto codeViewDebugEntry = PEHeaderParser::GetCodeViewDebugEntry(module);
        auto pdbFileName = codeViewDebugEntry->GetPdbFileName();
        auto il2cpp = strstr(pdbFileName, "_il2cpp_") != nullptr;
        auto development = strstr(pdbFileName, "_development_") != nullptr;
        Type = il2cpp
                ? (development ? UnityRuntimeType::IL2CPPDevelopment : UnityRuntimeType::IL2CPP)
                : (development ? UnityRuntimeType::MonoDevelopment : UnityRuntimeType::Mono);
    }
    else
    {
        module = GetModuleHandle("Unity.exe");
        Type = UnityRuntimeType::Editor;
    }
    RvaBase = module;
}