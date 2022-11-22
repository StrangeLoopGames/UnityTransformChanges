namespace UnityTransformChanges
{
    using System;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    public unsafe readonly struct NativeTransformHierarchy : INativeDataPtr
    {
        readonly Data* data;

        void* INativeDataPtr.Data => this.data;

        public int TransformsCount => this.data->TransformsCount;
        public ulong DirtyFlags => this.data->DirtyFlags;
        public ReadOnlySpan<NativeTransform> Transforms => new(this.data->Transforms, this.data->TransformsCount);
        public ReadOnlySpan<long> DirtyFlagsPerTransform => new(this.data->DirtyFlagsPerTransform, this.data->TransformsCount);
        public ReadOnlySpan<long> AllFlagsPerTransform => new(this.data->AllFlagsPerTransform, this.data->TransformsCount);

        [StructLayout(LayoutKind.Explicit)]
        struct Data
        {
            [FieldOffset(0x10)] public readonly int TransformsCount;
            [FieldOffset(0x30)] public readonly NativeTransform* Transforms;
            [FieldOffset(0x40)] public readonly ulong DirtyFlags;
            [FieldOffset(0x48)] public readonly ulong* DirtyFlagsPerTransform;
            [FieldOffset(0x50)] public readonly ulong* AllFlagsPerTransform;
        }
    }
}