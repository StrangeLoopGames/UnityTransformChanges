namespace UnityTransformChanges
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    public readonly struct TransformAccessReadonly
    {
        public readonly NativeTransformHierarchy Hierarchy;
        public readonly int TransformIndex;
    }
}