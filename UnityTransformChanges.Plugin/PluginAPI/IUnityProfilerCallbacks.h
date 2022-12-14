// Unity Native Plugin API copyright © 2015 Unity Technologies ApS
//
// Licensed under the Unity Companion License for Unity - dependent projects--see[Unity Companion License](http://www.unity3d.com/legal/licenses/Unity_Companion_License).
//
// Unless expressly provided otherwise, the Software under this license is made available strictly on an “AS IS” BASIS WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.Please review the license for details on these and other terms and conditions.

#pragma once
#include "IUnityInterface.h"

#include <stdint.h>

// Unity Profiler Native plugin API provides an ability to register callbacks for Unity Profiler events.
// The basic functionality includes:
// 1. Category callback - provides information about category name and color.
// 2. Profiler marker callback - provides information about new marker which is created internally or with C# API.
// 3. Marker event callback - allows to Transform_set_Position_interceptor begin/end events for the particular marker.
// 4. Frame event callback - provides information about logical CPU frame.
//
//  Usage example:
//
//  #include <IUnityInterface.h>
//  #include <IUnityProfilerCallbacks.h>
//
//  static IUnityProfilerCallbacks* s_UnityProfilerCallbacks = NULL;
//
//  static void UNITY_INTERFACE_API MyProfilerEventCallback(const UnityProfilerMarkerDesc* markerDesc, UnityProfilerMarkerEventType eventType, unsigned short eventDataCount, const UnityProfilerMarkerData* eventData, void* userData)
//  {
//      switch (eventType)
//      {
//          case kUnityProfilerMarkerEventTypeBegin:
//          {
//              MyProfilerPushMarker(markerDesc->name);
//              break;
//          }
//          case kUnityProfilerMarkerEventTypeEnd:
//          {
//              MyProfilerPopMarker(markerDesc->name);
//              break;
//          }
//      }
//  }
//
//  static void UNITY_INTERFACE_API MyProfilerCreateEventCallback(const UnityProfilerMarkerDesc* markerDesc, void* userData)
//  {
//      s_UnityProfilerCallbacks->RegisterEventCallback(markerDesc, MyProfilerEventCallback, NULL);
//  }
//
//  extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
//  {
//      s_UnityProfilerCallbacks = unityInterfaces->Get<IUnityProfilerCallbacks>();
//      s_UnityProfilerCallbacks->RegisterCreateEventCallback(&MyProfilerCreateEventCallback, NULL);
//  }
//
//  extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
//  {
//      s_UnityProfilerCallbacks->UnregisterCreateEventCallback(&MyProfilerCreateEventCallback, NULL);
//      s_UnityProfilerCallbacks->UnregisterEventCallback(NULL, &MyProfilerEventCallback, NULL);
//  }


typedef uint32_t UnityProfilerMarkerId;
typedef uint16_t UnityProfilerCategoryId;
typedef uint64_t UnityProfilerThreadId;

typedef struct UnityProfilerCategoryDesc
{
    // Incremental category index.
    UnityProfilerCategoryId id;
    // Reserved.
    uint16_t reserved0;
    // Internally associated category color which is in 0xRRGGBBAA format.
    uint32_t rgbaColor;
    // NULL-terminated string which is associated with the category.
    const char* name;
} UnityProfilerCategoryDesc;

enum UnityProfilerMarkerFlag_
{
    kUnityProfilerMarkerFlagDefault = 0,

    kUnityProfilerMarkerFlagScriptUser = 1 << 1,         // Markers created with C# API.
    kUnityProfilerMarkerFlagScriptInvoke = 1 << 5,       // Runtime invocations with ScriptingInvocation::Invoke.
    kUnityProfilerMarkerFlagScriptEnterLeave = 1 << 6,   // Deep profiler.

    kUnityProfilerMarkerFlagAvailabilityEditor = 1 << 2, // Editor-only marker, doesn't present in dev and non-dev players.
    kUnityProfilerMarkerFlagAvailabilityNonDev = 1 << 3, // Non-dev marker, present everywhere.

    kUnityProfilerMarkerFlagWarning = 1 << 4,            // Indicates undesirable, performance-wise suboptimal code path.

    kUnityProfilerMarkerFlagVerbosityDebug    = 1 << 10, // Internal debug markers - e.g. JobSystem Idle.
    kUnityProfilerMarkerFlagVerbosityInternal = 1 << 11, // Internal markers - e.g. Mutex/semaphore waits.
    kUnityProfilerMarkerFlagVerbosityAdvanced = 1 << 12  // Markers which are useful for advanced users - e.g. Loading.
};
typedef uint16_t UnityProfilerMarkerFlags;

enum UnityProfilerMarkerEventType_
{
    kUnityProfilerMarkerEventTypeBegin  = 0,
    kUnityProfilerMarkerEventTypeEnd    = 1,
    kUnityProfilerMarkerEventTypeSingle = 2
};
typedef uint16_t UnityProfilerMarkerEventType;

typedef struct UnityProfilerMarkerDesc
{
    // Per-marker callback chain pointer. Don't use.
    const void* callback;
    // Event id.
    UnityProfilerMarkerId id;
    // UnityProfilerMarkerFlag_ value.
    UnityProfilerMarkerFlags flags;
    // Category index the marker belongs to.
    UnityProfilerCategoryId categoryId;
    // NULL-terminated string which is associated with the marker.
    const char* name;
    // Metadata descriptions chain. Don't use.
    const void* metaDataDesc;
} UnityProfilerMarkerDesc;

enum UnityProfilerMarkerDataType_
{
    kUnityProfilerMarkerDataTypeNone = 0,
    kUnityProfilerMarkerDataTypeInstanceId = 1,
    kUnityProfilerMarkerDataTypeInt32 = 2,
    kUnityProfilerMarkerDataTypeUInt32 = 3,
    kUnityProfilerMarkerDataTypeInt64 = 4,
    kUnityProfilerMarkerDataTypeUInt64 = 5,
    kUnityProfilerMarkerDataTypeFloat = 6,
    kUnityProfilerMarkerDataTypeDouble = 7,
    kUnityProfilerMarkerDataTypeString = 8,
    kUnityProfilerMarkerDataTypeString16 = 9,
    kUnityProfilerMarkerDataTypeBlob8 = 11
};
typedef uint8_t UnityProfilerMarkerDataType;

typedef struct UnityProfilerMarkerData
{
    UnityProfilerMarkerDataType type;
    uint8_t reserved0;
    uint16_t reserved1;
    uint32_t size;
    const void* ptr;
} UnityProfilerMarkerData;

enum UnityProfilerFlowEventType_
{
    // The flow began withing a current marker scope (enclosed scope).
    kUnityProfilerFlowEventTypeBegin = 0,
    // The flow continues with the next sample.
    kUnityProfilerFlowEventTypeNext  = 1,
    // The flow ends with the next sample.
    kUnityProfilerFlowEventTypeEnd   = 2,
};
typedef uint8_t UnityProfilerFlowEventType;

typedef struct UnityProfilerThreadDesc
{
    // Thread id c-casted to uint64_t.
    uint64_t threadId;
    // Thread group name. NULL-terminated.
    const char* groupName;
    // Thread name. NULL-terminated.
    const char* name;
} UnityProfilerThreadDesc;

// Called when a new category is created. Up to 4 installed callbacks are supported.
// \param categoryDesc is a pointer to category description.
// \param userData is an optional context pointer passed with RegisterCreateCategoryCallback call.
typedef void (UNITY_INTERFACE_API * IUnityProfilerCreateCategoryCallback)(const UnityProfilerCategoryDesc* categoryDesc, void* userData);

// Called when a new profiler marker is created. Up to 4 installed callbacks are supported.
// \param markerDesc is a pointer to marker description.
// \param userData is an optional context pointer passed with RegisterCreateEventCallback call.
typedef void (UNITY_INTERFACE_API * IUnityProfilerCreateMarkerCallback)(const UnityProfilerMarkerDesc* markerDesc, void* userData);

// Called when a profiler event occurs.
// \param markerDesc is an marker description struct.
// \param eventType is an event type - UnityProfilerMarkerEventType_.
// \param eventDataCount is an event metadata count.
// \param eventData is a metadata array of eventDataCount elements.
// \param userData is an optional context pointer passed with RegisterCreateEventCallback call.
typedef void (UNITY_INTERFACE_API * IUnityProfilerMarkerEventCallback)(const UnityProfilerMarkerDesc* markerDesc, UnityProfilerMarkerEventType eventType, uint16_t eventDataCount, const UnityProfilerMarkerData* eventData, void* userData);

// Called when a profiler frame change occurs. Up to 4 installed callbacks are supported.
// \param userData is an optional context pointer passed with RegisterCreateEventCallback call.
typedef void (UNITY_INTERFACE_API * IUnityProfilerFrameCallback)(void* userData);

// Called when a profiler frame change occurs. Up to 4 installed callbacks are supported.
// \param userData is an optional context pointer passed with RegisterCreateEventCallback call.
typedef void (UNITY_INTERFACE_API * IUnityProfilerThreadCallback)(const UnityProfilerThreadDesc* threadDesc, void* userData);

// Called when a profiler flow event occurs.
// Flow events connect events across threads allowing to trace related activities initiated from a control(main) thread.
// \param flowEventType is a flow event type.
// \param flowId is an unique incremental identifier for the flow chain.
// \param userData is an optional context pointer passed with RegisterCreateEventCallback call.
typedef void (UNITY_INTERFACE_API * IUnityProfilerFlowEventCallback)(UnityProfilerFlowEventType flowEventType, uint32_t flowId, void* userData);

// Available since 2019.1
UNITY_DECLARE_INTERFACE(IUnityProfilerCallbacksV2)
{
    // Register a callback which is called when a new category is created.
    // Returns 0 on success and non-zero in case of error.
    // Only up to 4 callbacks can be registered.
    int(UNITY_INTERFACE_API * RegisterCreateCategoryCallback)(IUnityProfilerCreateCategoryCallback callback, void* userData);
    int(UNITY_INTERFACE_API * UnregisterCreateCategoryCallback)(IUnityProfilerCreateCategoryCallback callback, void* userData);

    // Register a callback which is called when a new marker is created.
    // E.g. dynamically created in C# with "new PerformanceMarker(string)" or internally.
    // Also is called for all existing markers when the callback is registered.
    // Returns 0 on success and non-zero in case of error.
    // Only up to 4 callbacks can be registered.
    int(UNITY_INTERFACE_API * RegisterCreateMarkerCallback)(IUnityProfilerCreateMarkerCallback callback, void* userData);
    int(UNITY_INTERFACE_API * UnregisterCreateMarkerCallback)(IUnityProfilerCreateMarkerCallback callback, void* userData);

    // Register a callback which is called every time "PerformanceMarker.Begin(string)" is called or equivalent internal native counterpart.
    // Returns 0 on success and non-zero in case of error.
    // \param markerDesc is a pointer to marker description you want to install callback for.
    int(UNITY_INTERFACE_API * RegisterMarkerEventCallback)(const UnityProfilerMarkerDesc * markerDesc, IUnityProfilerMarkerEventCallback callback, void* userData);
    // Unregister per-marker callback.
    // \param markerDesc is a pointer to marker description you want to remove callback from.
    // \param userData optional context pointer.
    // If markerDesc is NULL, callback is removed from all events which have (callback, userData) pair.
    // If both markerDesc and userData are NULL, callback is removed from all events which have callback function pointer.
    int(UNITY_INTERFACE_API * UnregisterMarkerEventCallback)(const UnityProfilerMarkerDesc * markerDesc, IUnityProfilerMarkerEventCallback callback, void* userData);

    // Register a callback which is called every time Profiler.BeginSample is called or equivalent internal native counterpart.
    // Returns 0 on success and non-zero in case of error.
    // Only up to 4 callbacks can be registered.
    int(UNITY_INTERFACE_API * RegisterFrameCallback)(IUnityProfilerFrameCallback callback, void* userData);
    int(UNITY_INTERFACE_API * UnregisterFrameCallback)(IUnityProfilerFrameCallback callback, void* userData);

    // Register a callback which is called every time a new thread is registered with profiler.
    // Returns 0 on success and non-zero in case of error.
    // Only up to 4 callbacks can be registered.
    int(UNITY_INTERFACE_API * RegisterCreateThreadCallback)(IUnityProfilerThreadCallback callback, void* userData);
    int(UNITY_INTERFACE_API * UnregisterCreateThreadCallback)(IUnityProfilerThreadCallback callback, void* userData);

    // Register a callback which is called every time Unity generates a flow event.
    // Returns 0 on success and non-zero in case of error.
    // Note: Flow events are supported only in Development Players and Editor.
    int(UNITY_INTERFACE_API * RegisterFlowEventCallback)(IUnityProfilerFlowEventCallback callback, void* userData);
    // Unregister flow event callback.
    // \param userData optional context pointer.
    int(UNITY_INTERFACE_API * UnregisterFlowEventCallback)(IUnityProfilerFlowEventCallback callback, void* userData);
};
UNITY_REGISTER_INTERFACE_GUID(0x5DEB59E88F2D4571ULL, 0x81E8583069A5E33CULL, IUnityProfilerCallbacksV2)

// Available since 2018.2
UNITY_DECLARE_INTERFACE(IUnityProfilerCallbacks)
{
    int(UNITY_INTERFACE_API * RegisterCreateCategoryCallback)(IUnityProfilerCreateCategoryCallback callback, void* userData);
    int(UNITY_INTERFACE_API * UnregisterCreateCategoryCallback)(IUnityProfilerCreateCategoryCallback callback, void* userData);

    int(UNITY_INTERFACE_API * RegisterCreateMarkerCallback)(IUnityProfilerCreateMarkerCallback callback, void* userData);
    int(UNITY_INTERFACE_API * UnregisterCreateMarkerCallback)(IUnityProfilerCreateMarkerCallback callback, void* userData);

    int(UNITY_INTERFACE_API * RegisterMarkerEventCallback)(const UnityProfilerMarkerDesc * markerDesc, IUnityProfilerMarkerEventCallback callback, void* userData);
    int(UNITY_INTERFACE_API * UnregisterMarkerEventCallback)(const UnityProfilerMarkerDesc * markerDesc, IUnityProfilerMarkerEventCallback callback, void* userData);

    int(UNITY_INTERFACE_API * RegisterFrameCallback)(IUnityProfilerFrameCallback callback, void* userData);
    int(UNITY_INTERFACE_API * UnregisterFrameCallback)(IUnityProfilerFrameCallback callback, void* userData);

    int(UNITY_INTERFACE_API * RegisterCreateThreadCallback)(IUnityProfilerThreadCallback callback, void* userData);
    int(UNITY_INTERFACE_API * UnregisterCreateThreadCallback)(IUnityProfilerThreadCallback callback, void* userData);
};
UNITY_REGISTER_INTERFACE_GUID(0x572FDB38CE3C4B1FULL, 0xA6071A9A7C4F52D8ULL, IUnityProfilerCallbacks)
