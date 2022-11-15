using System;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityTransformChanges.InternalAPIEngineBridge;

namespace UnityTransformChanges
{
    public static class TransformDebugUtils
    {
        const string DllName = "libUnityTransformChanges.dll";
        
        public static event Action<int> TransformChanged;
    
#if UNITY_2021_3_9 && UNITY_64 && (UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN)
        public static bool IsTrackingSupported => true;
#else
        public static bool IsTrackingSupported => false;
#endif
    
        public static bool TrackTransformChanges { set => SetTransformChangeCallbackEnabled(value); }

        public static Transform GetTransformByID(int transformID) => (Transform) UnityInternalsBridge.FindObjectFromInstanceID(transformID);

        static readonly OnTransformChangeDelegate OnTransformChangeCallback = OnTransformChange;
    
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void OnTransformChangeDelegate(IntPtr data);
    
        [DllImport(DllName)]
        static extern void SetTransformChangeCallback(OnTransformChangeDelegate callback);
    
        [AOT.MonoPInvokeCallback(typeof(Action<IntPtr>))]
        static unsafe void OnTransformChange(IntPtr dataPtr)
        {
            var data = (TransformChangedCallbackData*) dataPtr.ToPointer();
            var transform = data->transform;
            if (transform != null)
                TransformChanged?.Invoke(transform->InstanceID);
        }
    
        static void SetTransformChangeCallbackEnabled(bool isEnabled)
        {
            try
            {
                SetTransformChangeCallback(isEnabled ? OnTransformChangeCallback : null);
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
        };

        [StructLayout(LayoutKind.Sequential)]
        unsafe readonly struct TransformChangedCallbackData
        {
            public readonly NativeTransform* transform;
            public readonly NativeTransformHierarchy* hierarchy;
            public readonly void* extra;
            public readonly TransformChangeType type;
        };
    
        [StructLayout(LayoutKind.Explicit)]
        unsafe struct NativeTransform
        {
            [FieldOffset(0x08)] public int InstanceID;
#if UNITY_EDITOR
            [FieldOffset(0x78)] public NativeTransformHierarchy* Hierarchy;
#else
        [FieldOffset(0x50)] public NativeTransformHierarchy* Hierarchy;
#endif
        }
    
        [StructLayout(LayoutKind.Explicit)]
        struct NativeTransformHierarchy
        {
            [FieldOffset(0x40)] public long ID;
        }
    }
}