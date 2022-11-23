#nullable enable

using System;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityTransformChanges.InternalAPIEngineBridge;

namespace UnityTransformChanges
{
    using Unity.Profiling;

    public static unsafe class TransformDebugUtils
    {
        const string DllName = "libUnityTransformChanges.dll";

        public delegate void TransformChangeDelegate(NativeTransform transform, NativeTransformHierarchy hierarchyID, ref bool bypass);

        public static event TransformChangeDelegate? TransformChange;

#if UNITY_2021_3_9 && UNITY_64 && (UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN)
        public static bool IsTrackingSupported => true;
#else
        public static bool IsTrackingSupported => false;
#endif

        public static bool TrackTransformChanges
        {
            set => SetTransformChangeCallbackEnabled(value);
        }

        public static Transform GetTransformByID(int transformID) => (Transform)UnityInternalsBridge.FindObjectFromInstanceID(transformID);

        static readonly NativeTransformChangeDelegate NativeTransformChangeCallback = OnTransformChange;
        static readonly TransformChangeDispatchGetAndClearChangedAsBatchedJobsInternalCallbackDelegate GetAndClearChangedAsBatchedJobsCallback = OnGetAndClearChangedAsBatchedJobs;


        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate bool NativeTransformChangeDelegate(IntPtr data);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate bool TransformJobMethodDelegate(IntPtr job, int mayBeIndex, TransformAccessReadonly transform, void* unknown1, uint unknown2);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate bool TransformChangeDispatchGetAndClearChangedAsBatchedJobsInternalCallbackDelegate(void* transformChangeDispatch, ulong arg0, TransformJobMethodDelegate jobMethod, TransformAccessReadonly* transforms, ProfilerMarker profilerMarker, [MarshalAs(UnmanagedType.LPStr)] string? descriptor);

        [DllImport(DllName)]
        static extern void SetTransformChangeCallbacks(NativeTransformChangeDelegate? transformChangeCallback, TransformChangeDispatchGetAndClearChangedAsBatchedJobsInternalCallbackDelegate? getAndClearChangedBatchedJobs);

        [AOT.MonoPInvokeCallback(typeof(NativeTransformChangeDelegate))]
        static bool OnTransformChange(IntPtr dataPtr)
        {
            var bypass = true;
            var data = (TransformChangedCallbackData*)dataPtr.ToPointer();
            var transform = data->transform;
            var hierarchy = data->hierarchy;
            TransformChange?.Invoke(transform, hierarchy, ref bypass);
            return bypass;
        }

        [AOT.MonoPInvokeCallback(typeof(TransformChangeDispatchGetAndClearChangedAsBatchedJobsInternalCallbackDelegate))]
        static bool OnGetAndClearChangedAsBatchedJobs(void* transformChangeDispatch, ulong mask, TransformJobMethodDelegate jobMethod, TransformAccessReadonly* transforms, ProfilerMarker profilerMarker, [MarshalAs(UnmanagedType.LPStr)] string? descriptor)
        {
            var bypass = true;
            return bypass;
        }

        static void SetTransformChangeCallbackEnabled(bool isEnabled)
        {
            try
            {
                if (isEnabled)
                    SetTransformChangeCallbacks(NativeTransformChangeCallback, GetAndClearChangedAsBatchedJobsCallback);
                else
                    SetTransformChangeCallbacks(null, null);
            }
            catch (DllNotFoundException)
            {
                Debug.LogError($"Package UnityTransformChanges not installed properly. Missing {DllName}");
            }
        }

        enum TransformChangeType : uint
        {
            TransformChangeDispatch = 0,
            Position = 1
        }

        [StructLayout(LayoutKind.Sequential)]
        readonly struct TransformChangedCallbackData
        {
            public readonly NativeTransform transform;
            public readonly NativeTransformHierarchy hierarchy;
            public readonly void* extra;
            public readonly TransformChangeType type;
        }
    }
}
