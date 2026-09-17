import Utils;
import GPassBase;

namespace Monoworks 
{

	ConstantBuffer<CameraConstants> u_CameraConstants; // set 1

    [shader("vertex")]
    float4 vertexMain(VertexInput input, uniform ModelConstants pushModelConstants)
    {

        float4 offsetWorldPos = float4(input.Position, 1.0);
        float4 currentWorldPos = mul(pushModelConstants.CurrentModelMatrix, offsetWorldPos);

        return mul(u_CameraConstants.CurrentViewProjection, currentWorldPos);

    }

}