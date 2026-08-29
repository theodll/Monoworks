module Common;

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

}