// UnityTransformChanges.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "UnityTransformChanges.h"

#include <utility>
#include <vector>

#include "IUnityInterface.h"

enum UnityRuntimeType
{
    Editor,
    Mono,
    IL2CPP
};

struct UnityRuntime
{
public:
    void* RvaBase;
    UnityRuntimeType Type;

    UnityRuntime()
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
};

UnityRuntime CurrentUnityRuntime;

struct Vector3
{
    float x;
    float y;
    float z;
};

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

void Transform_set_Position_Intercept(void* transform, Vector3* position);

typedef bool (*OnTransformChangeCallback)(TransformChangedCallbackData* data);
typedef bool (*OnTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal)(void* transformChangeDispatch, uint64_t arg0, void (__cdecl*)(void* job, uint32_t mayBeIndex, void* transformAccessReadOnly, void* unknown1, uint32_t unknown2), void* transformAccesses, void* profilerMarker, char const* descriptor);

OnTransformChangeCallback onTransformChangeCallback = nullptr;
OnTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal onTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_InternalCallback = nullptr;

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

struct MemoryWriter
{
    virtual size_t GetDataSize() const = 0;

    virtual void WriteTo(uint8_t* dst) const = 0;

    void WriteProtectedTo(uint8_t* dst) const
    {
        auto dataSize = GetDataSize();
        DWORD oldProtect;
        VirtualProtect(dst, dataSize, PAGE_EXECUTE_READWRITE, &oldProtect);
        this->WriteTo(dst);
        VirtualProtect(dst, dataSize, oldProtect, &oldProtect);
    }
};

struct MovToRaxWriter : MemoryWriter
{
public:
    static const int OpCodesLength = 10;

    explicit MovToRaxWriter(const uint64_t value) : Value(value) { }

    const uint64_t Value;

    inline size_t GetDataSize() const override
    {
        return OpCodesLength;
    }

    void WriteTo(uint8_t* dst) const override
    {
        // movabs rax, Value
        dst[0] = 0x48;
        dst[1] = 0xb8;
        *(uint64_t*)&dst[2] = Value;
    }
};

struct FarJumpWriter : MemoryWriter
{
    MovToRaxWriter movWriter;
public:
    static const int OpCodesLength = MovToRaxWriter::OpCodesLength + 2;

    explicit FarJumpWriter(const void* targetAddress) : movWriter((uint64_t)targetAddress) { }

    inline size_t GetDataSize() const override
    {
        return OpCodesLength;
    }

    void WriteTo(uint8_t* dst) const override
    {
        movWriter.WriteTo(dst);
        dst += movWriter.GetDataSize();
        // jmp rax
        dst[0] = 0xff;
        dst[1] = 0xe0;
    }
};

void DebugBreakIfAttached()
{
    if (IsDebuggerPresent())
        DebugBreak();
}

struct PatchSequence : MemoryWriter
{
public:
    const std::vector<uint8_t> bytesToReplace;

    PatchSequence(std::initializer_list<uint8_t> bytesToReplace) : bytesToReplace(std::vector<uint8_t>(bytesToReplace)) { }

    size_t GetDataSize() const override
    {
        return bytesToReplace.size();
    }

    void WriteTo(uint8_t *dst) const override
    {
        std::copy(bytesToReplace.begin(), bytesToReplace.end(), dst);
    }
};

PatchSequence MovRbxRsp8({ 0x48, 0x89, 0x5c, 0x24, 0x8  });

PatchSequence PushRbxSubRsp20({ 0x40, 0x53, 0x48, 0x83, 0xec, 0x20 });

PatchSequence PushRdiSubRsp30({ 0x40, 0x57, 0x48, 0x83, 0xec, 0x30 });

PatchSequence MovR8Rsp18( { 0x4c, 0x89, 0x44, 0x24, 0x18 });

struct HookInfo
{
    static __declspec(naked) void InterceptorHook()
    {
        asm volatile(R"(
            movq        %rax, 0x08(%rsp)
            addq        $0x08, %rax
            movq        %rax, 0x10(%rsp)
            subq        $0x58, %rsp
            movq        %rcx, 0x38(%rsp) # save all volatile registers
            movq        %rdx, 0x40(%rsp)
            movq        %r8,  0x48(%rsp)
            movq        %r9,  0x50(%rsp)
            movq        0x80(%rsp), %rax # copy arg5
            movq        %rax, 0x20(%rsp)
            movq        0x88(%rsp), %rax # copy arg6
            movq        %rax, 0x28(%rsp)
            movq        0x60(%rsp), %rax # get saved address of HookInfo
            call        *(%rax)          # call the Hook
            mov         0x38(%rsp), %rcx # restore all volatile registers
            mov         0x40(%rsp), %rdx
            mov         0x48(%rsp), %r8
            mov         0x50(%rsp), %r9
            addq        $0x58, %rsp
            test        %rax, %rax
            jz          1f
            jmp         *0x10(%rsp)
         1: ret)");
    }

public:
    const void* Hook;
    uint8_t ResumeOpCodes[32];
    const void* InterceptorHookAddress = (void*)InterceptorHook; // need this for jmp in HookInvokeOpCodes using [rax + offset InterceptorHookAddress]
    uint8_t HookInvokeOpCodes[MovToRaxWriter::OpCodesLength + 3]; // mov + jmp [rax + InterceptorHookAddress]

    explicit HookInfo(const void* hook) : Hook(hook), ResumeOpCodes(), HookInvokeOpCodes()
    {
    }

    void Init(const PatchSequence& patchSequence, void* returnAddress)
    {
        FarJumpWriter farJumpWriter(returnAddress);
        patchSequence.WriteTo(ResumeOpCodes);
        farJumpWriter.WriteTo(ResumeOpCodes + patchSequence.GetDataSize());
        // Mark this code as executable, it located in data section and need explicit permissions
        DWORD oldProtect;
        VirtualProtect(ResumeOpCodes, patchSequence.GetDataSize() + farJumpWriter.GetDataSize(), PAGE_EXECUTE_READWRITE, &oldProtect);

        uint8_t* dst = HookInvokeOpCodes;
        MovToRaxWriter((uint64_t)this).WriteTo(dst);
        dst += MovToRaxWriter::OpCodesLength;
        // jmp [rax + offsetof(InterceptorHookAddress)]
        *dst++ = 0xFF;
        *dst++ = 0x60;
        *dst   = sizeof(Hook) + sizeof(ResumeOpCodes);
        VirtualProtect(HookInvokeOpCodes, sizeof(HookInvokeOpCodes), PAGE_EXECUTE_READWRITE, &oldProtect);
    }
};

struct PatchTarget
{
    const uint64_t Rva;
    PatchSequence PatchSequence;
    const void* Hook;

    PatchTarget(uint64_t rva, struct PatchSequence patchSequence, void* hook = nullptr) : Rva(rva), PatchSequence(std::move(patchSequence)), Hook(hook) { }
};

struct Patch
{
    void* farJumpAddress;
    HookInfo hookInfo;
    PatchTarget target;

    uint64_t GetPatchAddress() const
    {
        return (uint64_t)CurrentUnityRuntime.RvaBase + target.Rva;
    }

    inline static const PatchTarget& SelectTarget(const PatchTarget& editor, const PatchTarget& mono, const PatchTarget& il2cpp)
    {
        switch (CurrentUnityRuntime.Type) {

            case Editor:
                return editor;
            case Mono:
                return mono;
            case IL2CPP:
                return il2cpp;
            default:
                throw std::exception();
        }
    }

public:
    Patch(const PatchTarget& target, const void* defaultHook) : target(target), hookInfo(target.Hook != nullptr ? target.Hook : defaultHook), farJumpAddress(nullptr) { }

    Patch(const PatchTarget& editor, const PatchTarget& mono, const PatchTarget& il2cpp, const void* defaultHook) : Patch(SelectTarget(editor, mono, il2cpp), defaultHook) { }

    /*
     * We use that fact what functions aligned for 0x10 and gaps filled with 0xcc (int 3) op codes.
     * We can try to find a big enough gap (size <= 15)
     */
    static void* FindGap(void* start, size_t size)
    {
        auto aligned = (uint8_t*)start;

        auto address = aligned - 1;
        auto gapStart = aligned - size;
        do {
            if (*(uint8_t *) address != 0xcc)
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

    bool Apply()
    {
        // DebugBreakIfAttached();
        auto address = GetPatchAddress();
        const PatchSequence& patchSequence = target.PatchSequence;
        auto cmp = memcmp((void*)address, patchSequence.bytesToReplace.data(), patchSequence.GetDataSize());
        if (cmp != 0)
        {
            MessageBox(nullptr, "Failed to apply patch, because bytes to replace doesn't match", "Error", MB_OK);
            return false;
        }

        if (farJumpAddress == nullptr)
            farJumpAddress = FindGap((void*)address, FarJumpWriter::OpCodesLength);

        // jmp far_jump
        DWORD oldProtect;
        VirtualProtect((void*) address, patchSequence.GetDataSize(), PAGE_EXECUTE_READWRITE, &oldProtect);
        uint8_t patch[patchSequence.GetDataSize()];
        memset(patch, 0xcc, patchSequence.GetDataSize());
        auto offset = (int64_t)((uint64_t)farJumpAddress - address - 2); // jmp byte offset has 2 bytes size
        if (offset >= -128 && offset <= 127)
        {
            patch[0] = 0xeb;
            patch[1] = offset;
        }
        else
        {
            patch[0] = 0xe9;
            *(uint32_t*)(patch + 1) = offset - 3;                       // extra 3 bytes for 5 byte size jmp with 32-bit offset
        }
        memcpy((void*)address, patch, patchSequence.GetDataSize());
        VirtualProtect((void*) address, patchSequence.GetDataSize(), oldProtect, &oldProtect);

        void *resumeAt = (void *) (address + patchSequence.GetDataSize());
        hookInfo.Init(patchSequence, resumeAt);

        const FarJumpWriter &writer = FarJumpWriter(hookInfo.HookInvokeOpCodes);
        writer.WriteProtectedTo((uint8_t*)farJumpAddress);
        return true;
    }

    void Rollback()
    {
        auto address = GetPatchAddress();
        target.PatchSequence.WriteProtectedTo((uint8_t*)address);
    }
};

std::vector<Patch*> appliedPatches;

Patch patchTransform_set_Position(PatchTarget(0xd40300, MovRbxRsp8), PatchTarget(0xb84b00, MovRbxRsp8), PatchTarget(0xB89B80, MovRbxRsp8), (void *) Transform_set_Position_Intercept);

inline bool SameAddressSpace(uint64_t x, uint64_t y)
{
    return (x & 0xfffffff000000000) == (y & 0xfffffff000000000);
}

void* GetTransformForHierarchy(void* transformHierarchy, size_t offset, size_t candidatesCount...)
{
    va_list args;
    va_start(args, candidatesCount);
    void* transform;
    for (auto i = 0; i < candidatesCount; ++candidatesCount)
    {
        void* candidate = va_arg(args, void*);
        if (SameAddressSpace((uint64_t) candidate, (uint64_t)transformHierarchy) && *(void**)((uint64_t)candidate + offset) == transformHierarchy)
        {
            transform = candidate;
            break;
        }
    }
    va_end(args);
    return transform;
}

bool TransformChangeDispatch_QueueTransformChangeIfHasChanged_Intercept_Internal(void* transformQueue, void* transformHierarchy, size_t hierarchyFieldOffset)
{
    register void* _rbx asm("%rbx");
    register void* _rdi asm("%rdi");
    register void* _r12 asm("%r12");
    if (onTransformChangeCallback == nullptr)
        return true;

    TransformChangedCallbackData data(nullptr, transformHierarchy, nullptr, TransformChangeType::TransformChangeDispatch);
    data.transform = GetTransformForHierarchy(transformHierarchy, hierarchyFieldOffset, 3, _rbx, _rdi, _r12);

    auto bypass = onTransformChangeCallback(&data);
    DebugBreakIfAttached();
    return bypass;
}

bool TransformChangeDispatch_QueueTransformChangeIfHasChanged_Intercept(void* transformQueue, void* transformHierarchy)
{
    return TransformChangeDispatch_QueueTransformChangeIfHasChanged_Intercept_Internal(transformQueue, transformHierarchy, 0x50);
}

bool TransformChangeDispatch_QueueTransformChangeIfHasChanged_Editor_Intercept(void* transformQueue, void* transformHierarchy)
{
    return TransformChangeDispatch_QueueTransformChangeIfHasChanged_Intercept_Internal(transformQueue, transformHierarchy, 0x78);
}

bool TransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal_Hook(void* transformChangeDispatch, uint64_t arg0, void (__cdecl* jobMethod)(void* job, uint32_t mayBeIndex, void* transformAccessReadOnly, void* unknown1, uint32_t unknown2), void* transformAccesses, void* profilerMarker, char const* descriptor)
{
    if (::onTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_InternalCallback == nullptr)
        return true;
    return ::onTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_InternalCallback(transformChangeDispatch, arg0, jobMethod, transformAccesses, profilerMarker, descriptor);
}

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
    if (patchTransformChangeDispatch_QueueTransformChangeIfHasChanged.Apply())
        appliedPatches.push_back(&patchTransformChangeDispatch_QueueTransformChangeIfHasChanged);
    if (patchTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal.Apply())
        appliedPatches.push_back(&patchTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal);
}

void RollbackPatches()
{
    for (auto patch : appliedPatches)
        patch->Rollback();
    appliedPatches.clear();
}

class MonoAssembly
{
    HMODULE hModule;

    void* (*mono_domain_get)();
    void* (*mono_domain_assembly_open)(void *domain, const char *name);
    void* (*mono_assembly_get_image)(void *assembly);
    void* (*mono_method_desc_new)(const char *name, int32_t include_namespace);
    void* (*mono_method_desc_search_in_image)(void *desc, void *image);
    void (*mono_method_desc_free)(void *desc);
    void* (*mono_lookup_internal_call)(void *monoMethod);
    void (*mono_add_internal_call)(const char *name, const void* method);

    void* image;

public:
    bool Init(const char* assemblyPath)
    {
        hModule = LoadLibrary("mono-2.0-bdwgc.dll");
        if (hModule == nullptr)
            return false;

        mono_domain_get = (void* (*)()) GetProcAddress(hModule, "mono_domain_get");
        mono_domain_assembly_open = (void* (*)(void *domain, const char *name))GetProcAddress(hModule, "mono_domain_assembly_open");
        mono_assembly_get_image = (void* (*)(void *assembly))GetProcAddress(hModule, "mono_assembly_get_image");
        mono_method_desc_new = (void* (*)(const char *name, int32_t include_namespace))GetProcAddress(hModule, "mono_method_desc_new");
        mono_method_desc_search_in_image = (void* (*)(void *desc, void *image))GetProcAddress(hModule, "mono_method_desc_search_in_image");
        mono_method_desc_free = (void (*)(void *desc))GetProcAddress(hModule, "mono_method_desc_free");
        mono_lookup_internal_call = (void* (*)(void *monoMethod))GetProcAddress(hModule, "mono_lookup_internal_call");
        mono_add_internal_call = (void (*)(const char *name, const void* method))GetProcAddress(hModule, "mono_add_internal_call");

        auto domain = mono_domain_get();
        auto assembly = mono_domain_assembly_open(domain, assemblyPath);
        image = mono_assembly_get_image(assembly);

        return true;
    }

    void* LookupInternalMethod(const char* methodSpec)
    {
        auto desc = mono_method_desc_new(methodSpec, 1);
        auto method = mono_method_desc_search_in_image(desc, image);
        auto ptr = mono_lookup_internal_call(method);
        mono_method_desc_free(desc);
        return ptr;
    }

    void Dispose()
    {
        if (hModule != nullptr)
            FreeLibrary(hModule);
        hModule = nullptr;
    }
};

void* SearchAndResolveByRelativeCall(void* caller, uint64_t offset)
{
    auto callAddress = SearchByPattern(caller, 0xe8 | (offset << 8),  0xffffffffff, 1024);
    return callAddress != nullptr ? (void*)((uint64_t) callAddress + offset + 5) : nullptr;
}

void DebugTransforms()
{
    auto assembly = (MonoAssembly*) std::malloc(sizeof(MonoAssembly));
    auto inited = assembly->Init("D:/projects/testing/UnityTest/Build VS/build/bin/UnityTest_Data/Managed/UnityEngine.CoreModule.dll");
    if (inited)
    {
        auto monoFuncAddress = assembly->LookupInternalMethod("UnityEngine.Transform:set_position_Injected");

        if (monoFuncAddress != nullptr)
        {
            auto rvaBasePlayer = GetModuleRVABase("UnityPlayer.dll");
            auto rvaBaseEditor = GetModuleRVABase("Unity.exe");

            uint64_t rvaBase;
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
                          (uint64_t) funcAddress - rvaBase, rvaBase);
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
    std::free(assembly);
}

void Transform_set_Position_Intercept(void* transform, Vector3* position)
{
    /*if (IsDebuggerPresent())
        DebugBreak();*/
    //MessageBox(nullptr, "OMG", "Test", MB_OK);
    TransformChangedCallbackData data(transform, nullptr, position, TransformChangeType::Position);
    if (onTransformChangeCallback != nullptr)
        onTransformChangeCallback(&data);
    //RollbackPatches();
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTransformChangeCallbacks(OnTransformChangeCallback onTransformChange, OnTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_Internal onGetAndClearBatchesJob)
{
    RollbackPatches();
    ::onTransformChangeCallback = onTransformChange;
    ::onTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_InternalCallback = onGetAndClearBatchesJob;
    if (::onTransformChangeCallback != nullptr || ::onTransformChangeDispatch_GetAndClearChangedAsBatchedJobs_InternalCallback != nullptr)
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
