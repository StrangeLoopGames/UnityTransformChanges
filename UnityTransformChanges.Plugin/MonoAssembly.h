//
// Created by mirasrael on 11/25/2022.
//

#ifndef UNITYTRANSFORMCHANGES_MONOASSEMBLY_H
#define UNITYTRANSFORMCHANGES_MONOASSEMBLY_H

#include "pch.h"

class MonoAssembly
{
    HINSTANCE hModule;

    void* (*mono_domain_get)();
    void* (*mono_domain_assembly_open)(void *domain, const char *name);
    void* (*mono_assembly_get_image)(void *assembly);
    void* (*mono_method_desc_new)(const char *name, int include_namespace);
    void* (*mono_method_desc_search_in_image)(void *desc, void *image);
    void (*mono_method_desc_free)(void *desc);
    void* (*mono_lookup_internal_call)(void *monoMethod);
    void (*mono_add_internal_call)(const char *name, const void* method);

    void* image;

public:
    bool Init(const char* assemblyPath);

    void* LookupInternalMethod(const char* methodSpec);

    void Dispose();
};

#endif //UNITYTRANSFORMCHANGES_MONOASSEMBLY_H
