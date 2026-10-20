import Utils;
import GPassBase;

namespace Monoworks
{

    ParameterBlock<MaterialInput> u_MaterialInput; 
    ConstantBuffer<CameraConstants> u_CameraConstants; 
    [vk::push_constant] ConstantBuffer<ModelConstants> pc_ModelConstants;


    struct DefaultMaterial : IMaterial {};

    static DefaultMaterial g_Material;

    /*
    [shader("vertex")]
    VertexOutput vertexMain(VertexInput input)
    {
        VertexOutput output;

        float4 position = float4(input.Position, 1.0);

        float4 currentWorldPos = mul( pc_ModelConstants.CurrentModelMatrix, position );

        output.CurrentClipPosition = mul( u_CameraConstants.CurrentViewProjection, currentWorldPos );

        float4 previousWorldPos = mul( pc_ModelConstants.PreviousModelMatrix, position );

        output.PreviousClipPosition = mul( u_CameraConstants.PreviousViewProjection, previousWorldPos );

        output.WorldPosition = currentWorldPos.xyz;
        output.WorldNormal = mul( (float3x3)pc_ModelConstants.CurrentModelMatrix, input.Normal);
        output.WorldTangent = mul( (float3x3)pc_ModelConstants.CurrentModelMatrix, input.Tangent);
        output.WorldBinormal = mul( (float3x3)pc_ModelConstants.CurrentModelMatrix, input.Binormal);

        output.TexCoord = input.TexCoord;
        output.EntityID = pc_ModelConstants.EntityID;

        return output;
    }*/

    
    [shader("vertex")]
    VertexOutput vertexMain( VertexInput input )
    {
        VertexOutput output;
        output = GPass::CalculateVertexOutput(input, g_Material, u_CameraConstants, pc_ModelConstants );
        return output;
    }

    [shader("fragment")]
    GBuffer fragmentMain(PixelInput input )
    {
        GBuffer gbuffer;

        MaterialSurface surface = g_Material.ComputeSurface(u_MaterialInput, input);
        gbuffer = GPass::CalculateGBuffer(surface, input, u_CameraConstants, pc_ModelConstants);
        return gbuffer;
    }

}