using System;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityTransformChanges.InternalAPIEngineBridge;

namespace UnityTransformChanges
{
    public static class TransformDebugUtils
    {
        const string DllName = "libUnityTransformChanges.dll";

        public delegate void TransformChangeDelegate(NativeTransform transform, NativeTransformHierarchy hierarchyID, ref bool bypass);

        public static event TransformChangeDelegate TransformChange;

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

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate bool NativeTransformChangeDelegate(IntPtr data);

        [DllImport(DllName)]
        static extern void SetTransformChangeCallback(NativeTransformChangeDelegate callback);

        [AOT.MonoPInvokeCallback(typeof(NativeTransformChangeDelegate))]
        static unsafe bool OnTransformChange(IntPtr dataPtr)
        {
            var bypass = true;
            var data = (TransformChangedCallbackData*)dataPtr.ToPointer();
            var transform = data->transform;
            var hierarchy = data->hierarchy;
            TransformChange?.Invoke(transform, hierarchy, ref bypass);
            return bypass;
        }

        static void SetTransformChangeCallbackEnabled(bool isEnabled)
        {
            try
            {
                SetTransformChangeCallback(isEnabled ? NativeTransformChangeCallback : null);
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
        unsafe readonly struct TransformChangedCallbackData
        {
            public readonly NativeTransform transform;
            public readonly NativeTransformHierarchy hierarchy;
            public readonly void* extra;
            public readonly TransformChangeType type;
        }
    }
}
