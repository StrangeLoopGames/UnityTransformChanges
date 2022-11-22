namespace UnityTransformChanges
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    public unsafe readonly struct NativeTransform : INativeDataPtr
    {
        readonly Data* data;

        void* INativeDataPtr.Data => this.data;

        public int InstanceID => this.data->InstanceID;
        public NativeTransformHierarchy Hierarchy => this.data->Hierarchy;

        [StructLayout(LayoutKind.Explicit)]
        struct Data
        {
            [FieldOffset(0x08)] public int InstanceID;
#if UNITY_EDITOR
            [FieldOffset(0x78)] public NativeTransformHierarchy Hierarchy;
#else
            [FieldOffset(0x50)] public NativeTransformHierarchy Hierarchy;
#endif
        }
    }
}