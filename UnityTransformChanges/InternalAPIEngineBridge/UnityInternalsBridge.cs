using UnityEngine;

namespace UnityTransformChanges.InternalAPIEngineBridge
{
    internal static class UnityInternalsBridge
    {
        public static Object FindObjectFromInstanceID(int instanceID) => Object.FindObjectFromInstanceID(instanceID);
    }
}