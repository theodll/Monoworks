import Utils;

module GPassBase;


namespace Monoworks
{    

    // Constants
    public struct CameraConstants // It's a camera constant.
    {
        float4x4            CurrentViewProjection;
        float4x4            PreviousViewProjection;
        float4x4            InverseViewProjection; // Current too.
        float3              CameraPosition;
    }

    public struct ModelConstants // It's a push constant, not a ConstantBuffer like CameraConstants. It's just for unified naming.
    {
        float4x4            CurrentModelMatrix;
        float4x4            PreviousModelMatrix;
        uint                EntityID;    
    };

    public struct MaterialInputUBO
    {
        public float3 AlbedoVector;
        public float3 EmissiveColor;
        public float Metallicness;
        public float Roughness;
        public float EmissionFactor;
        public float EnviromentMapRotation; // NOTE: Not yet implemented.
        public float AmbientOcclusionFactor;
        public bool UseNormalMap;
        public uint MaterialID; // NOTE: Not yet implemented.
    }


	public struct MaterialInput
    {

        ConstantBuffer<MaterialInputUBO> UBO;

        public Texture2D    AlbedoMap;
        public Texture2D    NormalMap;
        public Texture2D    EmissiveMap;
        public Texture2D    RoughnessMap;
        public Texture2D    MetallicMap;
        public Texture2D    OcclusionMap;
        public SamplerState MapSampler;
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
        public float4       CurrentClipPosition    : SV_Position;
        public float4       PreviousClipPosition : PREV_Position;
        public float3       WorldPosition   : WORLD_POSITION;
        public float3       WorldNormal     : WORLD_NORMAL;
        public float3       WorldTangent    : WORLD_TANGENT;
        public float3       WorldBinormal   : WORLD_BINORMAL;
        public float2       TexCoord        : TEXCOORD0;
        public uint         EntityID        : ENTITYID0;
    }

    public struct PixelInput
    {
        public float4       CurrentClipPosition : SV_Position;
        public float4       PreviousClipPosition : PREV_Position;
        public float3       WorldPosition   : WORLD_POSITION;
        public float3       WorldNormal     : WORLD_NORMAL;
        public float3       WorldTangent    : WORLD_TANGENT;
        public float3       WorldBinormal   : WORLD_BINORMAL;
        public float2       TexCoord        : TEXCOORD0;
        public uint         EntityID        : ENTITYID0;
    }

    public struct GBuffer // 19 Bytes Color + 4 Bytes Depth
    {
        // NOTE: Vulkan 1.3 only garantees a maximum of 4 rendertargets bound at the same time but I don't care.
        // Maybe  
        float4 AlbedoOcclusion : SV_Target0;   // R8B8G8A8_UNORM      // RGB = albedo, A = occlusion
        float4 NormalRoughMetal : SV_Target1; // A2B10G10R10_UNORM   // A is metal. RG is normal. B is roughness.
        float3 Emissive : SV_Target2;        // R8G8B8_UNORM  // RGB = emissive
        float2 MotionVector : SV_Target3;   // R16G16_SFLOAT  

        // NOTE: This uses some really freaky bit packing:
        // Bits 0-18 are assigned to the entity id. 2^19 = ~525k different Entity IDs (if you use more than that you got a serious issue)
        // Bits 19-31 are assigned to the material id. 2^13 = 8192 different Material IDs (this value might be a bit small but eight-thousand different IDs is enough at first)
        uint EntityMaterialID : SV_Target4;  // R32_UINT 
    }

    public struct MaterialSurface
    {
        public float3       AlbedoColor;
        public float3       Normal;
        public float        Metallic = 0.0;
        public float        Roughness = 0.5;
        public float3       Emissive = float3(0);
        public float OcclusionFactor = 1.0f;
        public uint MaterialID; 
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
            MaterialSurface surface;

            var albedoMap    =  materialInput.AlbedoMap.Sample(materialInput.MapSampler, pixelInput.TexCoord);
            var normalMap    =  materialInput.NormalMap.Sample(materialInput.MapSampler, pixelInput.TexCoord);
            var roughnessMap =  materialInput.RoughnessMap.Sample(materialInput.MapSampler, pixelInput.TexCoord);
            var metallicMap  =  materialInput.MetallicMap.Sample(materialInput.MapSampler, pixelInput.TexCoord);
            var occlusionMap =  materialInput.OcclusionMap.Sample(materialInput.MapSampler, pixelInput.TexCoord);
            var emissiveMap  =  materialInput.EmissiveMap.Sample(materialInput.MapSampler, pixelInput.TexCoord);

            surface.AlbedoColor = albedoMap.rgb * materialInput.UBO.AlbedoVector.rgb;
            surface.Emissive = emissiveMap.rgb * materialInput.UBO.EmissiveColor * materialInput.UBO.EmissionFactor;
            surface.Metallic = metallicMap.b * materialInput.UBO.Metallicness; // NOTE: its the Blue color chanel in the glTF standart
            surface.Roughness = roughnessMap.g * materialInput.UBO.Roughness; // NOTE: its the Green color chanel in the glTF standart
            surface.OcclusionFactor = occlusionMap.r * materialInput.UBO.AmbientOcclusionFactor; // NOTE: its the red chanel in the glTF standart
            
            float3 T = pixelInput.WorldTangent;
            float3 B = pixelInput.WorldBinormal;
            float3 N = pixelInput.WorldNormal;

            if (length(T) < 0.0000001)
            {
                T = cross(N, float3(0.0, 1.0, 0.0));
                if (length(T) < 0.00000001)
                    T = cross(N, float3(1.0, 0.0, 0.0));
            }

            T = normalize(T - dot(T, N) * N);

            if (length(B) < 0.00000001)
                B = cross(N, T) * pixelInput.WorldTangent;
            else
                B = normalize(B - dot(B, N) * N);

            if (dot(cross(N, T), pixelInput.WorldBinormal) < 0.0)
            {
                B = -B;
            }

            float3x3 TBN = float3x3(T, B, N);

            float3 tangentNormal = normalMap.rgb * 2.0 - 1.0;

            float3 worldNormal = normalize(mul(tangentNormal, TBN));

            surface.Normal = worldNormal;
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
        public static VertexOutput CalculateVertexOutput<T>( in VertexInput vertexInput, in T material, in CameraConstants cameraConstants, in ModelConstants modelConstants ) 
                where T : IMaterial
        {
            VertexOutput output;

            float4 offsetWorldPos = float4(vertexInput.Position, 1.0) + material.CalculateVertexOffset(vertexInput);

            float4 currentWorldPos = mul(modelConstants.CurrentModelMatrix, offsetWorldPos);
            output.WorldPosition = currentWorldPos.xyz;
            output.CurrentClipPosition = mul(cameraConstants.CurrentViewProjection, currentWorldPos );

            float4 previousWorldPos = mul(modelConstants.PreviousModelMatrix, offsetWorldPos);
            output.PreviousClipPosition = mul(cameraConstants.PreviousViewProjection, previousWorldPos);

            float3x3 normalMat = (float3x3)modelConstants.CurrentModelMatrix;
            output.WorldNormal = mul(normalMat, vertexInput.Normal);
            output.WorldBinormal = mul(normalMat, vertexInput.Binormal);
            output.WorldTangent = mul(normalMat, vertexInput.Tangent);

            output.TexCoord = vertexInput.TexCoord;
            output.EntityID = modelConstants.EntityID;

            return output;
        }

        public static GBuffer CalculateGBuffer(in MaterialSurface materialSurface, in PixelInput pixelInput, in CameraConstants cameraConstants, in ModelConstants modelConstants)
        {
            GBuffer gbuffer;

            gbuffer.AlbedoOcclusion.rgb = materialSurface.AlbedoColor;
            gbuffer.AlbedoOcclusion.a = materialSurface.OcclusionFactor;

            gbuffer.NormalRoughMetal.rg = OctEncode(materialSurface.Normal);
            gbuffer.NormalRoughMetal.b = materialSurface.Roughness;
            gbuffer.NormalRoughMetal.a = materialSurface.Metallic;

            gbuffer.Emissive = materialSurface.Emissive;

            gbuffer.EntityMaterialID = (materialSurface.MaterialID & 0x1FFFu) | ((modelConstants.EntityID & 0x7FFFFu) << 13);

            float2 currentNDC = pixelInput.CurrentClipPosition.xy / pixelInput.CurrentClipPosition.w * 0.5 + 0.5;
            float2 previousNDC = pixelInput.PreviousClipPosition.xy / pixelInput.PreviousClipPosition.w * 0.5 + 0.5;

            gbuffer.MotionVector = currentNDC - previousNDC;            
        }

    }

}