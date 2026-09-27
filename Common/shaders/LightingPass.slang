import Utils;
import GPassBase;

namespace Monoworks
{
	ConstantBuffer<CameraConstants> u_CameraConstants;
	ParameterBlock<GBufferPB> u_GBuffer;

	[shader("compute")]
    [numthreads(16, 16, 1)]
    func LightMain(uint3 dispatchThreadID: SV_DispatchThreadID ) -> void 
	{

        u_GBuffer.Composite[dispatchThreadID.xy] = float4(u_GBuffer.AlbedoOcclusion[dispatchThreadID.xy].rgb, 1.0f);

	}

}
