#nullable enable

using System;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityTransformChanges.InternalAPIEngineBridge;

namespace UnityTransformChanges
{
    using Unity.Profiling;
    using Debug = UnityEngine.Debug;

    public static unsafe class TransformDebugUtils
    {
        const string DllName = "libUnityTransformChanges.dll";

        public delegate void TransformChangeDelegate(NativeTransform transform, NativeTransformHierarchy hierarchyID, ref bool bypass);
        public delegate void TransformChangeAppliedInJobDelegate(ReadOnlySpan<TransformAccessReadonly> transforms, ulong mask);
        public delegate void TransformChangesApplyStartedDelegate(string? descriptor, ulong mask);
        public delegate void TransformChangesApplyFinishedDelegate();

        public static event TransformChangeDelegate? TransformChange;
        public static event TransformChangeAppliedInJobDelegate? TransformChangeAppliedInJob;
        public static event TransformChangesApplyStartedDelegate? TransformChangesApplyStarted;
        public static event TransformChangesApplyFinishedDelegate? TransformChangesApplyFinished;

#if UNITY_2021_3_9 && UNITY_64 && (UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN)
        public static bool IsTrackingSupported => true;
#else
        public static bool IsTrackingSupported => false;
#endif

        public static bool TrackTransformChanges
        {
            set => SetTransformChangeCallbackEnabled(value);
        }

        public static Transform? GetTransformByID(int transformID) => (Transform?)UnityInternalsBridge.FindObjectFromInstanceID(transformID);

        static readonly NativeTransformChangeDelegate NativeTransformChangeCallback = OnTransformChange;
        static readonly TransformChangeDispatchGetAndClearChangedAsBatchedJobsInternalCallbackDelegate GetAndClearChangedAsBatchedJobsCallback = OnGetAndClearChangedAsBatchedJobs;
        static readonly TransformJobMethodDelegate TransformJobMethodHook = TransformJobMethodHandler;

        static TransformJobMethodDelegate? currentTransformJobMethod ;

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate bool NativeTransformChangeDelegate(IntPtr data);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void TransformJobMethodDelegate(IntPtr job, int mayBeIndex, TransformAccessReadonly* transform, in ulong mask, int transformsCount);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void TransformChangeDispatchGetAndClearChangedAsBatchedJobsInternalDelegate(void* transformChangeDispatch, ulong mask, TransformJobMethodDelegate? jobMethod, void* jobData, ProfilerMarker profilerMarker, [MarshalAs(UnmanagedType.LPStr)] string? descriptor);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void TransformChangeDispatchGetAndClearChangedAsBatchedJobsInternalCallbackDelegate(void* transformChangeDispatch, ulong mask, TransformJobMethodDelegate jobMethod, void* jobData, ProfilerMarker profilerMarker, [MarshalAs(UnmanagedType.LPStr)] string? descriptor, TransformChangeDispatchGetAndClearChangedAsBatchedJobsInternalDelegate originalFunction);

        [DllImport(DllName)]
        static extern void SetTransformChangeCallbacks(NativeTransformChangeDelegate? transformChangeCallback, TransformChangeDispatchGetAndClearChangedAsBatchedJobsInternalCallbackDelegate? getAndClearChangedBatchedJobs);

        [AOT.MonoPInvokeCallback(typeof(NativeTransformChangeDelegate))]
        static bool OnTransformChange(IntPtr dataPtr)
        {
            var bypass = true;
            var data = (TransformChangedCallbackData*)dataPtr.ToPointer();
            var transform = data->transform;
            var hierarchy = data->hierarchy;
            try
            {
                TransformChange?.Invoke(transform, hierarchy, ref bypass);
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
            }
            return bypass;
        }

        [AOT.MonoPInvokeCallback(typeof(TransformChangeDispatchGetAndClearChangedAsBatchedJobsInternalCallbackDelegate))]
        static void OnGetAndClearChangedAsBatchedJobs(void* transformChangeDispatch, ulong mask, TransformJobMethodDelegate? jobMethod, void* jobData, ProfilerMarker profilerMarker, [MarshalAs(UnmanagedType.LPStr)] string? descriptor, TransformChangeDispatchGetAndClearChangedAsBatchedJobsInternalDelegate originalFunction)
        {
            if (TransformChangeAppliedInJob != null)
            {
                currentTransformJobMethod = jobMethod;
                jobMethod = TransformJobMethodHook;
            }

            try
            {
                TransformChangesApplyStarted?.Invoke(descriptor, mask);
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
            }
            originalFunction(transformChangeDispatch, mask, jobMethod, jobData, profilerMarker, descriptor);
            try
            {
                TransformChangesApplyFinished?.Invoke();
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
            }
        }

        [AOT.MonoPInvokeCallback(typeof(TransformJobMethodDelegate))]
        static void TransformJobMethodHandler(IntPtr job, int mayBeIndex, TransformAccessReadonly* transforms, in ulong mask, int transformsCount)
        {
            currentTransformJobMethod!(job, mayBeIndex, transforms, mask, transformsCount);
            try
            {
                TransformChangeAppliedInJob?.Invoke(new ReadOnlySpan<TransformAccessReadonly>(transforms, transformsCount), mask);
            }
            catch (Exception)
            {
                // ignored
            }
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
