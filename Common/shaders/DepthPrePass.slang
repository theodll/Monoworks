import Utils;
import GPassBase;

namespace Monoworks
{

    // Global-scope UBO so CGraphicsPrePass::BindUBO (set 0) can write it.
    ConstantBuffer<CameraConstants> u_CameraConstants;
    [vk::push_constant] ConstantBuffer<ModelConstants> pc_ModelConstants;

    [shader("vertex")]
    float4 vertexMain(VertexInput input )
    {

        float4 offsetWorldPos = float4(input.Position, 1.0);
        float4 currentWorldPos = mul(pc_ModelConstants.CurrentModelMatrix, offsetWorldPos);

        return mul(u_CameraConstants.CurrentViewProjection, currentWorldPos);

    }

}