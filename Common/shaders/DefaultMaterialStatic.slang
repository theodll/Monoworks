import Utils;
import GPassBase;

namespace Monoworks
{

    ParameterBlock<MaterialInput> u_MaterialInput; // NOTE: set 2 // Must be named like this.
    ParameterBlock<ConstantBuffer<CameraConstants>> u_CameraConstants; // NOTE: set 1
    [vk::push_constant] ConstantBuffer<ModelConstants> pc_ModelConstants;


    struct DefaultMaterial : IMaterial {};

    static DefaultMaterial g_Material;
    
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