//
// Created by mirasrael on 11/25/2022.
//

#include "pch.h"
#include "MonoAssembly.h"

bool MonoAssembly::Init(const char *assemblyPath) {
    hModule = LoadLibrary("mono-2.0-bdwgc.dll");
    if (hModule == nullptr)
        return false;

    mono_domain_get = (void* (*)()) GetProcAddress(hModule, "mono_domain_get");
    mono_domain_assembly_open = (void* (*)(void *domain, const char *name))GetProcAddress(hModule, "mono_domain_assembly_open");
    mono_assembly_get_image = (void* (*)(void *assembly))GetProcAddress(hModule, "mono_assembly_get_image");
    mono_method_desc_new = (void* (*)(const char *name, int include_namespace))GetProcAddress(hModule, "mono_method_desc_new");
    mono_method_desc_search_in_image = (void* (*)(void *desc, void *image))GetProcAddress(hModule, "mono_method_desc_search_in_image");
    mono_method_desc_free = (void (*)(void *desc))GetProcAddress(hModule, "mono_method_desc_free");
    mono_lookup_internal_call = (void* (*)(void *monoMethod))GetProcAddress(hModule, "mono_lookup_internal_call");
    mono_add_internal_call = (void (*)(const char *name, const void* method))GetProcAddress(hModule, "mono_add_internal_call");

    auto domain = mono_domain_get();
    auto assembly = mono_domain_assembly_open(domain, assemblyPath);
    image = mono_assembly_get_image(assembly);

    return true;
}

void *MonoAssembly::LookupInternalMethod(const char *methodSpec) {
    auto desc = mono_method_desc_new(methodSpec, 1);
    auto method = mono_method_desc_search_in_image(desc, image);
    auto ptr = mono_lookup_internal_call(method);
    mono_method_desc_free(desc);
    return ptr;
}

void MonoAssembly::Dispose() {
    if (hModule != nullptr)
        FreeLibrary(hModule);
    hModule = nullptr;
}
