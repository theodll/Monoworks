module GPassBase;


namespace Monoworks
{    

    // Constants
    public struct CameraConstants // It's a camera constant
    {
        float4x4           ViewProjection;
        float4x4           InverseViewProjection;
        float3             CameraPosition;
    }

    public struct ModelConstants // It's a push constant, not a ConstantBuffer like CameraConstants. It's just for unified naming.
    {
        float4x4            Transform;
        uint                EntityID;    
    };

	public struct MaterialInput
    {
        public float3       AlbedoVector;
        public float3       EmissiveColor;
        public float        Metallicness;
        public float        Roughness;
        public float        EmissionFactor;
        public float        EnviromentMapRotation;   // NOTE: Not yet implemented
        public float        AmbientOcclusionFactor;  
        public bool         UseNormalMap;
        public uint         MaterialID;

        public Texture2D    AlbedoMap;
        public Texture2D    NormalMap;
        public Texture2D    RoughnessMap;
        public Texture2D    MetallicMap;
        public Texture2D    OcclusionMap;
        public Sampler2D    MapSampler;
	}

    public struct VertexInput
    {
        public float3       Position         : POSITION;
        public float3       Normal           : NORMAL;
        public float3       Tangent          : TANGENT;
        public float3       Binormal         : BINORMAL;
        public float2       TexCoord         : TEXCOORD0;
    }

    public struct VertexOutput
    {
        public float4       ClipPosition    : SV_Position;
        public float3       WorldPosition   : WORLD_POSITION;
        public float3       WorldNormal     : WORLD_NORMAL;
        public float3       WorldTangent    : WORLD_TANGENT;
        public float3       WorldBinormal   : WORLD_BINORMAL;
        public float2       TexCoord        : TEXCOORD0;
        public uint         EntityID        : ENTITYID0;
        public uint         MaterialID      : MATERIALID0;
    }

    public struct PixelInput
    {
        public float4       ClipPosition    : SV_Position;
        public float3       WorldPosition   : WORLD_POSITION;
        public float3       WorldNormal     : WORLD_NORMAL;
        public float3       WorldTangent    : WORLD_TANGENT;
        public float3       WorldBinormal   : WORLD_BINORMAL;
        public float2       TexCoord        : TEXCOORD0;
        public uint         EntityID        : ENTITYID0;
        public uint         MaterialID      : MATERIALID0;
    }

    public struct GBufferOutput 
    {
        float4 AlbedoOcclusion : SV_Target0; // R8B8G8A8_UNORM // xyz = albedo, w = occlusion
        float2 MotionVector : SV_Target1; // R16G16_SFLOAT // used for TAA/Motion Blur

        uint    EntityID : SV_Target3;              // R32_UINT
        uint    MaterialID : SV_Target4;            // R32_UINT
    }

    public struct MaterialSurface
    {
        public float3       AlbedoColor;
        public float3       Normal;
        public float        Metallic = 0.0;
        public float        Roughness = 0.5;
        public float3       Emissive = float3(0);
        public float        OcclusionFactor = 1.0f;
    }

    public interface IMaterial
    {
        public float4 CalculateVertexOffset( in VertexInput input )
        {
            return float4(0);
        };

        /**
         *   @brief This function computes the Material Surface Properties.
         *   This function (if not overwritten) computes the default material surface properties and returns it.
         *   The individual elements can optionally be overwritten by the functions down below, if the user chooses not to
         *   write a custom implementation to calculate all elements of the MaterialSurface struct.
         *   @return MaterialSurface All material surface properties calculated by the implementation of the function
         */
        public MaterialSurface ComputeSurface( in MaterialInput materialInput, in PixelInput pixelInput) 
        {

        };

        public float4           ComputeColor(           in MaterialInput materialInput, in PixelInput pixelInput );
        public float4           ComputeNormal(          in MaterialInput materialInput, in PixelInput pixelInput );
        public float            ComputeMetallicness(    in MaterialInput materialInput, in PixelInput pixelInput );
        public float            ComputeRoughness(       in MaterialInput materialInput, in PixelInput pixelInput );
        public float3           ComputeEmissiveColor(   in MaterialInput materialInput, in PixelInput pixelInput );
        public float            ComputeOcclusionFactor( in MaterialInput materialInput, in PixelInput pixelInput );

    }

    public struct GPass
    {
        public static VertexOutput CalculateVertexOutput<T>(in VertexInput vertexInput, inout T material, in CameraConstants cameraConstants, in ModelConstants modelConstants ) where T : IMaterial 
        {

        }

        

    }

}