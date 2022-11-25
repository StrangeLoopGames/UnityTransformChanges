//
// Created by mirasrael on 11/25/2022.
//

#ifndef UNITYTRANSFORMCHANGES_UNITYRUNTIME_H
#define UNITYTRANSFORMCHANGES_UNITYRUNTIME_H

enum UnityRuntimeType
{
    Editor,
    Mono,
    IL2CPP
};

class UnityRuntime {
public:
    void* RvaBase;
    UnityRuntimeType Type;

    UnityRuntime();
};

extern UnityRuntime CurrentUnityRuntime;

#endif //UNITYTRANSFORMCHANGES_UNITYRUNTIME_H
