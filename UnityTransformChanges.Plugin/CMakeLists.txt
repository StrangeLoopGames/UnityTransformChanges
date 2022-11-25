include_directories(PluginAPI)

add_library(UnityTransformChanges SHARED
        PluginAPI/IUnityEventQueue.h
        PluginAPI/IUnityGraphics.h
        PluginAPI/IUnityGraphicsD3D11.h
        PluginAPI/IUnityGraphicsD3D12.h
        PluginAPI/IUnityGraphicsMetal.h
        PluginAPI/IUnityGraphicsVulkan.h
        PluginAPI/IUnityInterface.h
        PluginAPI/IUnityProfilerCallbacks.h
        PluginAPI/IUnityRenderingExtensions.h
        PluginAPI/IUnityShaderCompilerAccess.h
        framework.h
        UnityTransformChanges.cpp UnityTransformChanges.h
        UnityRuntime.cpp UnityRuntime.h
        Writers/MemoryWriter.cpp Writers/MemoryWriter.h
        Writers/MovToRaxWriter.cpp Writers/MovToRaxWriter.h
        Writers/FarJumpWriter.cpp Writers/FarJumpWriter.h
        HookInfo.cpp HookInfo.h
        PatchSequence.cpp PatchSequence.h
        PatchTarget.cpp PatchTarget.h
        Patch.cpp Patch.h
        MonoAssembly.cpp MonoAssembly.h
        Debug.cpp Debug.h)

if (APPLE)
    target_precompile_headers(UnityTransformChanges PRIVATE pch.h)
endif()

if (WIN32)
    target_sources(UnityTransformChanges PRIVATE dllmain.cpp)
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS} -Wl,-undefined,dynamic_lookup")
endif()

add_custom_command(TARGET UnityTransformChanges POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy
    "$<TARGET_FILE:UnityTransformChanges>"
    "${PROJECT_SOURCE_DIR}/UnityTransformChanges/Plugins/x86_64/$<TARGET_FILE_NAME:UnityTransformChanges>"
    COMMENT "Copying to output directory"
)