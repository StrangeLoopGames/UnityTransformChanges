//
// Created by mirasrael on 11/25/2022.
//

#include "UnityRuntime.h"
#include "pch.h"

UnityRuntime CurrentUnityRuntime;

UnityRuntime::UnityRuntime()
{
    RvaBase = GetModuleHandle("UnityPlayer.dll");
    if (RvaBase != nullptr)
    {
        Type = GetModuleHandle("mono-2.0-bdwgc.dll") != nullptr ? UnityRuntimeType::Mono : UnityRuntimeType::IL2CPP;
    }
    else
    {
        RvaBase = GetModuleHandle("Unity.exe");
        Type = UnityRuntimeType::Editor;
    }
}