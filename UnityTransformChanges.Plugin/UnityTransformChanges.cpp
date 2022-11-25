// UnityTransformChanges.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "UnityTransformChanges.h"

#include <utility>
#include <vector>

#include "IUnityInterface.h"
#include "UnityRuntime.h"
#include "Writers/MemoryWriter.h"
#include "Writers/MovToRaxWriter.h"
#include "Writers/FarJumpWriter.h"
#include "Debug.h"
#include "PatchSequence.h"
#include "HookInfo.h"
#include "Patch.h"
#include "PatchTarget.h"

enum TransformChangeType : uint32_t
{
    TransformChangeDispatch = 0,
    Position = 1
};

struct TransformChangedCallbackData
{
    void* transform;
    void* hierarchy;
    void* extra;
    TransformChangeType type;
public:
    TransformChangedCallbackData(void* transform, void* hierarchy, void* extra, TransformChangeType type) : transform(transform), hierarchy(hierarchy), extra(extra), type(type) { }
};

typedef bool (*OnTransformChangeCallback)(TransformChangedCallbackData* data);
typedef void (*TransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal)(void* transformChangeDispatch, uint64_t arg0, void (__cdecl*)(void* job, uint32_t mayBeIndex, void* transformAccessReadOnly, uint64_t* unknown1, uint32_t unknown2), void* transformAccesses, void* profilerMarker, char const* descriptor);
typedef void (*OnTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal)(void* transformChangeDispatch, uint64_t arg0, void (__cdecl*)(void* job, uint32_t mayBeIndex, void* transformAccessReadOnly, uint64_t* unknown1, uint32_t unknown2), void* transformAccesses, void* profilerMarker, char const* descriptor, TransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal originalFunction);

OnTransformChangeCallback onTransformChangeCallback = nullptr;
OnTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal onTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_InternalCallback = nullptr;

PatchSequence MovRbxRsp8({ 0x48, 0x89, 0x5c, 0x24, 0x8  });
PatchSequence PushRbxSubRsp20({ 0x40, 0x53, 0x48, 0x83, 0xec, 0x20 });
PatchSequence PushRdiSubRsp30({ 0x40, 0x57, 0x48, 0x83, 0xec, 0x30 });
PatchSequence MovR8Rsp18( { 0x4c, 0x89, 0x44, 0x24, 0x18 });

std::vector<Patch*> appliedPatches;

inline bool SameAddressSpace(uint64_t x, uint64_t y)
{
    return (x & 0xfffffff000000000) == (y & 0xfffffff000000000);
}

void* GetTransformForHierarchy(void* transformHierarchy, size_t offset, size_t candidatesCount...)
{
    va_list args;
    va_start(args, candidatesCount);
    void* transform = nullptr;
    for (auto i = 0; i < candidatesCount; ++i)
    {
        void* candidate = va_arg(args, void*);
        void* hierarchyCandidate;
        if (SameAddressSpace((uint64_t) candidate, (uint64_t)transformHierarchy)
            && ReadProcessMemory(GetCurrentProcess(), (void*)((uint64_t)candidate + offset), &hierarchyCandidate, sizeof(hierarchyCandidate), nullptr)
            && hierarchyCandidate == transformHierarchy)
        {
            transform = candidate;
            break;
        }
    }
    va_end(args);
    return transform;
}

#define INTERCEPTOR_HOOK_START(ret, methodName, ...) ret methodName(__VA_ARGS__) { \
    register ret (*_rax)(__VA_ARGS__) asm("%rax"); \
    auto originalFunction = _rax;

#define INTERCEPTOR_HOOK_END() }

void TransformChangeDispatch_QueueTransformChangeIfHasChanged_Intercept_Internal(void* transformQueue, void* transformHierarchy, size_t hierarchyFieldOffset, void (*originalFunction)(void*, void*))
{
    register void* _rbx asm("%rbx");
    register void* _rdx asm("%rdx");
    register void* _rdi asm("%rdi");
    register void* _r12 asm("%r12");
    if (onTransformChangeCallback != nullptr)
    {
        TransformChangedCallbackData data(nullptr, transformHierarchy, nullptr, TransformChangeType::TransformChangeDispatch);
        data.transform = GetTransformForHierarchy(transformHierarchy, hierarchyFieldOffset, 4, _rbx, _rdx, _rdi, _r12);

        auto bypass = onTransformChangeCallback(&data);
        DebugBreakIfAttached();
        if (bypass)
            originalFunction(transformQueue, transformHierarchy);
    }
}

INTERCEPTOR_HOOK_START(void, TransformChangeDispatch_QueueTransformChangeIfHasChanged_Intercept, void* transformQueue, void* transformHierarchy)
    TransformChangeDispatch_QueueTransformChangeIfHasChanged_Intercept_Internal(transformQueue, transformHierarchy, 0x50, originalFunction);
INTERCEPTOR_HOOK_END()

INTERCEPTOR_HOOK_START(void, TransformChangeDispatch_QueueTransformChangeIfHasChanged_Editor_Intercept, void* transformQueue, void* transformHierarchy)
    TransformChangeDispatch_QueueTransformChangeIfHasChanged_Intercept_Internal(transformQueue, transformHierarchy, 0x78, originalFunction);
INTERCEPTOR_HOOK_END()

INTERCEPTOR_HOOK_START(void, TransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal_Hook,
                       void *transformChangeDispatch, uint64_t mask, void
                       (__cdecl *jobMethod)(void *jobData, uint32_t mayBeIndex, void *transformAccessesReadonly, uint64_t *mask, uint32_t transformsCount),
                       void *jobData, void *profilerMarker, char const *descriptor)
    if (::onTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_InternalCallback != nullptr)
        ::onTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_InternalCallback(transformChangeDispatch, mask, jobMethod, jobData, profilerMarker, descriptor, originalFunction);
    else
        originalFunction(transformChangeDispatch, mask, jobMethod, jobData, profilerMarker, descriptor);
INTERCEPTOR_HOOK_END()


Patch patchTransformChangeDispatch_QueueTransformChangeIfHasChanged(
        PatchTarget(0xD3B960, PushRdiSubRsp30, (void *) TransformChangeDispatch_QueueTransformChangeIfHasChanged_Editor_Intercept),
        PatchTarget(0xB7B110, PushRbxSubRsp20),
        PatchTarget(0xB80190, PushRbxSubRsp20),
        (void *) TransformChangeDispatch_QueueTransformChangeIfHasChanged_Intercept);

Patch patchTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal(
        PatchTarget(0x00d37aa0, MovR8Rsp18),
        PatchTarget(0x0, MovR8Rsp18),
        PatchTarget(0x0, MovR8Rsp18),
        (void *) TransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal_Hook);

uint64_t GetModuleRVABase(const char* dllName)
{
    return (uint64_t)GetModuleHandle(dllName);
}

void ApplyPatches()
{
    if (onTransformChangeCallback != nullptr && patchTransformChangeDispatch_QueueTransformChangeIfHasChanged.Apply())
        appliedPatches.push_back(&patchTransformChangeDispatch_QueueTransformChangeIfHasChanged);
    if (onTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_InternalCallback != nullptr && patchTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal.Apply())
        appliedPatches.push_back(&patchTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal);
}

void RollbackPatches()
{
    for (auto patch : appliedPatches)
        patch->Rollback();
    appliedPatches.clear();
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTransformChangeCallbacks(OnTransformChangeCallback onTransformChange, OnTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal onGetAndClearBatchesJob)
{
    RollbackPatches();
    ::onTransformChangeCallback = onTransformChange;
    ::onTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_InternalCallback = onGetAndClearBatchesJob;
    ApplyPatches();
}

// Unity plugin load event
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces * unityInterfaces)
{
}

// Unity plugin unload event
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginUnload()
{
    SetTransformChangeCallbacks(nullptr, nullptr);
}
