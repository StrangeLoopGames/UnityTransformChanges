// UnityTransformChanges.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "UnityTransformChanges.h"

#include "IUnityInterface.h"

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
bool (*onTransformChangeCallback)(TransformChangedCallbackData* data) = nullptr;

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

struct FarJumpWriter : MemoryWriter
{
public:
    static const int OpCodesLength = 12;

    explicit FarJumpWriter(const void* targetAddress) : TargetAddress(targetAddress) { }

    const void* TargetAddress;

    inline size_t GetDataSize() const override
    {
        return OpCodesLength;
    }

    void WriteTo(uint8_t* dst) const override
    {
        // movabs rax, TargetAddress
        dst[0] = 0x48;
        dst[1] = 0xb8;
        *(const void**)(dst + 2) = TargetAddress;
        // jmp rax
        dst[10] = 0xff;
        dst[11] = 0xe0;
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
    const uint8_t* bytesToReplace;
    const size_t bytesCount;

    explicit PatchSequence(const uint8_t* bytesToReplace, size_t bytesCount) : bytesToReplace(bytesToReplace), bytesCount(bytesCount) { }

    size_t GetDataSize() const override
    {
        return bytesCount;
    }

    void WriteTo(uint8_t *dst) const override
    {
        memcpy(dst, bytesToReplace, bytesCount);
    }
};

const uint8_t MovRbxRsp8Bytes[5] = { 0x48, 0x89, 0x5c, 0x24, 0x8  };
PatchSequence MovRbxRsp8(MovRbxRsp8Bytes, sizeof(MovRbxRsp8Bytes));

const uint8_t PushRbxSubRsp20Bytes[6] = { 0x40, 0x53, 0x48, 0x83, 0xec, 0x20 };
PatchSequence PushRbxSubRsp20(PushRbxSubRsp20Bytes, sizeof(PushRbxSubRsp20Bytes));

const uint8_t PushRdiSubRsp30Bytes[6] = { 0x40, 0x57, 0x48, 0x83, 0xec, 0x30 };
PatchSequence PushRdiSubRsp30(PushRdiSubRsp30Bytes, sizeof(PushRdiSubRsp30Bytes));

struct HookInfo
{
public:
    const void* Hook;
    uint8_t ResumeOpCodes[32];
    PatchSequence PatchSequence;

    HookInfo(const void* hook, const struct PatchSequence& patchSequence) : Hook(hook), PatchSequence(patchSequence), ResumeOpCodes()
    {
        patchSequence.WriteTo(ResumeOpCodes);
    }

    void SetReturnAddress(void* returnAddress)
    {
        FarJumpWriter(returnAddress).WriteTo(ResumeOpCodes + PatchSequence.bytesCount);
    }
};

struct Patch
{
    const uint64_t rva;
    void** returnAddress;
    void* farJumpAddress;

    uint64_t GetPatchAddress(void* rvaBase) const
    {
        return (uint64_t)rvaBase + rva;
    }

public:
    HookInfo Hook;

    Patch(const uint64_t rva, const PatchSequence& patchSequence, const void *interceptor, const void* hook, void** returnAddress) : rva(rva), Hook(hook, patchSequence), returnAddress(returnAddress), farJumpAddress(nullptr) {
    }

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

    void* FarJumpPatch(void* patchAddress) const
    {
        // far_jump: movabs rax, interceptor
        // jmp rax
        const FarJumpWriter &writer = FarJumpWriter(Hook.Hook);
        void* gap = FindGap((void*)patchAddress, writer.GetDataSize());
        writer.WriteProtectedTo((uint8_t*)gap);
        return gap;
    }

    void Apply(void* rvaBase)
    {
        // DebugBreakIfAttached();
        auto address = GetPatchAddress(rvaBase);
        auto cmp = memcmp((void*)address, Hook.PatchSequence.bytesToReplace, Hook.PatchSequence.bytesCount);
        if (cmp != 0)
        {
            MessageBox(nullptr, "Failed to apply patch, because bytes to replace doesn't match", "Error", MB_OK);
            return;
        }

        if (farJumpAddress == nullptr)
            farJumpAddress = FarJumpPatch((void*)address);

        // jmp far_jump
        DWORD oldProtect;
        VirtualProtect((void*) address, Hook.PatchSequence.bytesCount, PAGE_EXECUTE_READWRITE, &oldProtect);
        uint8_t patch[Hook.PatchSequence.bytesCount];
        memset(patch, 0xcc, Hook.PatchSequence.bytesCount);
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
        memcpy((void*)address, patch, Hook.PatchSequence.bytesCount);
        VirtualProtect((void*) address, Hook.PatchSequence.bytesCount, oldProtect, &oldProtect);

        void *resumeAt = (void *) (address + Hook.PatchSequence.bytesCount);
        *returnAddress = resumeAt;
        Hook.SetReturnAddress(resumeAt);
    }

    void Rollback(void* rvaBase)
    {
        auto address = GetPatchAddress(rvaBase);

        Hook.PatchSequence.WriteProtectedTo((uint8_t*)address);
    }
};

#define INTERCEPTOR_MOV_RBX_RSP_8(FunctionName)                                                                               \
void* FunctionName##_return; \
__declspec(naked) void FunctionName##Interceptor()                                                              \
{                                                                                                               \
    asm("movq        %%rbx, 0x8(%%rsp)\n\r"  \
        "movq        %%rcx, 0x10(%%rsp)\n\r" \
        "movq        %%rdx, 0x18(%%rsp)\n\r" \
        "push        %%rbp\n\r" \
        "mov         %%rsp, %%rbp\n\r" \
        "subq        $0x20, %%rsp\n\r" \
        "lea         %1, %%rax\n\r" \
        "call        *%%rax\n\r" \
        "addq        $0x20, %%rsp\n\r" \
        "pop         %%rbp\n\r" \
        "mov         0x10(%%rsp), %%rcx\n\r" \
        "mov         0x18(%%rsp), %%rdx\n\r" \
        "movq        %0, %%rax\n\r" \
        "jmp         *%%rax"::"m"(FunctionName##_return),"m"(FunctionName##_Intercept)); \
}

#define INTERCEPTOR_PUSH_RBX_SUB_RSP_20(FunctionName) \
void* FunctionName##_return; \
__declspec(naked) void FunctionName##Interceptor() \
{ \
    asm("push        %%rbx\n\r" \
        "subq        $0x40, %%rsp\n\r" \
        "movq        %%rcx, 0x20(%%rsp)\n\r" \
        "movq        %%rdx, 0x28(%%rsp)\n\r" \
        "lea         %1, %%rax\n\r" \
        "call        *%%rax\n\r" \
        "test        %%rax, %%rax\n\r" \
        "jz          1f\n\r" \
        "mov         0x20(%%rsp), %%rcx\n\r" \
        "mov         0x28(%%rsp), %%rdx\n\r" \
        "addq        $0x20, %%rsp\n\r" \
        "movq        %0, %%rax\n\r" \
        "jmp         *%%rax\n\r" \
        "1:\n\r" \
        "addq        $0x40, %%rsp\n\r" \
        "pop         %%rbx\n\r" \
        "ret\n\r"::"m"(FunctionName##_return),"m"(FunctionName##_Intercept)); \
}

#define INTERCEPTOR_PUSH_RDI_SUB_RSP_30(FunctionName) \
void* FunctionName##_return; \
__declspec(naked) void FunctionName##Interceptor() \
{ \
    asm("movq        %1, %%rax\n\r" \
        "subq        $0x48, %%rsp\n\r" \
        "movq        %%rcx, 0x20(%%rsp)\n\r" \
        "movq        %%rdx, 0x28(%%rsp)\n\r" \
        "movq        %%r8,  0x30(%%rsp)\n\r" \
        "movq        %%r9,  0x38(%%rsp)\n\r" \
        "call        *%%rax\n\r" \
        "mov         0x20(%%rsp), %%rcx\n\r" \
        "mov         0x28(%%rsp), %%rdx\n\r" \
        "mov         0x30(%%rsp), %%r8\n\r" \
        "mov         0x38(%%rsp), %%r9\n\r" \
        "addq        $0x48, %%rsp\n\r" \
        "test        %%rax, %%rax\n\r" \
        "jz          1f\n\r" \
        "push        %%rdi\n\r" \
        "subq        $0x30, %%rsp\n\r"\
        "movq        %0, %%rax\n\r" \
        "jmp         *%%rax\n\r" \
        "1:\n\r" \
        "ret\n\r"::"m"(FunctionName##_return),"m"(FunctionName##_hook->Hook)); \
}

INTERCEPTOR_MOV_RBX_RSP_8(Transform_set_Position)

Patch playerPatch(0xb84b00, MovRbxRsp8, (void *) Transform_set_PositionInterceptor, (void *) Transform_set_Position_Intercept, &Transform_set_Position_return);
Patch il2cppPatch(0xB89B80, MovRbxRsp8, (void *) Transform_set_PositionInterceptor, (void *) Transform_set_Position_Intercept, &Transform_set_Position_return);
Patch editorPatch(0xd40300, MovRbxRsp8, (void *) Transform_set_PositionInterceptor, (void *) Transform_set_Position_Intercept, &Transform_set_Position_return);

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

extern HookInfo* TransformChangeDispatch_QueueTransformChangeIfHasChanged_Editor_hook;

INTERCEPTOR_PUSH_RBX_SUB_RSP_20(TransformChangeDispatch_QueueTransformChangeIfHasChanged)
INTERCEPTOR_PUSH_RDI_SUB_RSP_30(TransformChangeDispatch_QueueTransformChangeIfHasChanged_Editor)

Patch il2cppDispatchPatch(0xB80190, PushRbxSubRsp20, (void *) TransformChangeDispatch_QueueTransformChangeIfHasChangedInterceptor, (void *) TransformChangeDispatch_QueueTransformChangeIfHasChanged_Intercept, &TransformChangeDispatch_QueueTransformChangeIfHasChanged_return);
Patch monoDispatchPatch(0xB7B110, PushRbxSubRsp20, (void *) TransformChangeDispatch_QueueTransformChangeIfHasChangedInterceptor, (void *) TransformChangeDispatch_QueueTransformChangeIfHasChanged_Intercept, &TransformChangeDispatch_QueueTransformChangeIfHasChanged_return);
Patch editorDispatchPatch(0xD3B960, PushRdiSubRsp30, (void *) TransformChangeDispatch_QueueTransformChangeIfHasChanged_EditorInterceptor, (void *) TransformChangeDispatch_QueueTransformChangeIfHasChanged_Editor_Intercept, &TransformChangeDispatch_QueueTransformChangeIfHasChanged_Editor_return);
HookInfo* TransformChangeDispatch_QueueTransformChangeIfHasChanged_Editor_hook = &editorDispatchPatch.Hook;

Patch* activePatch;

uint64_t GetModuleRVABase(const char* dllName)
{
    return (uint64_t)GetModuleHandle(dllName);
}

void ApplyPatches()
{
    auto rvaBasePlayer = GetModuleRVABase("UnityPlayer.dll");
    auto rvaBaseEditor = GetModuleRVABase("Unity.exe");
    auto monoRuntime = GetModuleHandle("mono-2.0-bdwgc.dll") != nullptr;

    if (rvaBasePlayer != 0)
    {
        activePatch = &(monoRuntime ? monoDispatchPatch : il2cppDispatchPatch);
        activePatch->Apply(reinterpret_cast<void *>(rvaBasePlayer));
    }
    else if (rvaBaseEditor != 0)
    {
        activePatch = &editorDispatchPatch;
        activePatch->Apply(reinterpret_cast<void *>(rvaBaseEditor));
    }
    else
        MessageBox(nullptr, "Failed to patch Transform.set_Position, no UnityPlayer.dll", "Test", MB_OK);
}

void RollbackPatches()
{
    auto rvaBasePlayer = GetModuleRVABase("UnityPlayer.dll");
    auto rvaBaseEditor = GetModuleRVABase("Unity.exe");

    if (rvaBasePlayer != 0)
        activePatch->Rollback(reinterpret_cast<void *>(rvaBasePlayer));
    else if (rvaBaseEditor != 0)
        activePatch->Rollback(reinterpret_cast<void *>(rvaBaseEditor));
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

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTransformChangeCallback(bool (*onTransformChange)(TransformChangedCallbackData* transform))
{
    if (::onTransformChangeCallback != nullptr)
        RollbackPatches();
    ::onTransformChangeCallback = onTransformChange;
    if (::onTransformChangeCallback != nullptr)
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
    SetTransformChangeCallback(nullptr);
}