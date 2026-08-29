module Base;

namespace Monoworks 
{
    public struct VertexInput 
    {
        public float3 Position  : POSITION;
        public float3 Normal    : NORMAL;
        public float3 Tangent   : TANGENT;
        public float3 Binormal  : BINORMAL;
        public float2 TexCoord  : TEXCOORD0;
    }

    public struct CameraConstants
    {
        float4x4    ViewProjection;
        float4x4    InverseViewProjection;
        float3      CameraPosition;
    }
}