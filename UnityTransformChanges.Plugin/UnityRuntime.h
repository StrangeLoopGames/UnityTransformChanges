//
// Created by mirasrael on 11/25/2022.
//

#ifndef UNITYTRANSFORMCHANGES_UNITYRUNTIME_H
#define UNITYTRANSFORMCHANGES_UNITYRUNTIME_H

enum UnityRuntimeType
{
    Editor,
    Mono,
    MonoDevelopment,
    IL2CPP,
    IL2CPPDevelopment
};

class UnityRuntime {
public:
    void* RvaBase;
    UnityRuntimeType Type;

    UnityRuntime();
};

extern UnityRuntime CurrentUnityRuntime;

#endif //UNITYTRANSFORMCHANGES_UNITYRUNTIME_H
