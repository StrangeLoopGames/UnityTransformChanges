namespace UnityTransformChanges
{
    public static class NativeDataPtrExtensions
    {
        public static unsafe bool IsNull(this INativeDataPtr ptr) => ptr.Data == null;
    }
}