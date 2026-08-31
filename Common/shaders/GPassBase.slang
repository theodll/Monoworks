module GPassBase;


namespace Monoworks
{    
    public struct CameraConstants
    {
        float4x4           ViewProjection;
        float4x4           InverseViewProjection;
        float3             CameraPosition;
    }

	public struct MaterialInput
    {
        public float3       AlbedoVector;
        public float3       EmissiveColor;
        public float        Metallicness;
        public float        Roughness;
        public float        EmissionFactor;
        public float        EnviromentMapRotation;   // TODO: Implement
        public float        AmbientOcclusionFactor;  
        public bool         UseNormalMap;

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

    public struct MaterialSurface
    {
        public float3 AlbedoColor;
        public float3 Normal;
        public float Metallic = 0.0;
        public float Roughness = 0.5;
        public float3 Emissive = float3(0);
        public float OcclusionFactor = 1.0f;
    }

    public interface IMaterial
    {
        public float4 CalculateVertexOffset(VertexInput input)
        {
            return float4(0);
        };

        public MaterialSurface ComputeSurface(MaterialInput materialInput, PixelInput pixelInput);

        public float4 ComputeColor(MaterialInput materialInput, PixelInput pixelInput);
        public float4 ComputeNormal(MaterialInput materialInput, PixelInput pixelInput);
        public float ComputeMetallicness(MaterialInput materialInput, PixelInput pixelInput);
        public float ComputeRoughness( MaterialInput materialInput, PixelInput pixelInput);
        public float3 ComputeEmissiveColor(MaterialInput materialInput, PixelInput pixelInput);
        public float ComputeOcclusionFactor(MaterialInput materialInput, PixelInput pixelInput);

    }

}