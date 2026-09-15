import Utils;
import GPassBase;

namespace Monoworks
{

    ParameterBlock<CameraConstants> u_CameraConstants; // NOTE: set 1
    ParameterBlock<MaterialInput> u_MaterialInput; // NOTE: set 2
    
    static IMaterial g_Material;
    
    [shader("vertex")]
    VertexOutput vertexMain( VertexInput input, uniform ModelConstants pushModelConstants )
    {
        VertexOutput output;
        output = GPass::CalculateVertexOutput(input, g_Material, u_CameraConstants, pushModelConstants );
        return output;
    }

    [shader("fragment")]
    GBuffer fragmentMain(PixelInput input, uniform ModelConstants pushModelConstants)
    {
        GBuffer gbuffer;

        MaterialSurface surface = g_Material.ComputeSurface(u_MaterialInput, input);
        gbuffer = GPass::CalculateGBuffer(surface, input, u_CameraConstants, pushModelConstants);
        return gbuffer;
    }




}