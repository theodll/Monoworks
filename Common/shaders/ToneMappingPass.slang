import Utils;
import GPassBase;

namespace Monoworks
{

	enum TonemapMode 
	{
		MW_TONEMAP_MODE_NONE, // Pass through
		MW_TONEMAP_MODE_REINHARD, 
		MW_TONEMAP_MODE_REINHARD_EXTENDED,
        MW_TONEMAP_MODE_REINHARD_JODIE,
        MW_TONEMAP_MODE_HABLE_FILMIC,
		MW_TONEMAP_MODE_ACES_FILMIC,
		MW_TONEMAP_MODE_AGX,
		MW_TONEMAP_MODE_KHRONOS_PBR_NEUTRAL
	}

	public struct TonemapParams
    {
        TonemapMode Mode;
        float Exposure;
        uint EnableGamma;
	}

    RWTexture2D u_SwapchainImage;
    ConstantBuffer<TonemapParams> u_Params;

	ParameterBlock<GBufferPB> u_GBuffer;

    func Luminance(in float3 color) -> float
    {
        return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
    }

    func ChangeLuminance(in float3 colorIn, in float luminanceOut) -> float3
    {
        float luminanceIn = Luminance(colorIn);
        return colorIn * (luminanceOut / luminanceIn);
    }

    func Linear( in float3 color) -> float3
    {
        return color;
	}

    func pReinhard( in float3 color) -> float3
    {
        float lOld = Luminance(color);
        float lNew = lOld / (1.0f + lOld);
        return ChangeLuminance( color, lNew );
	}

    func ReinhardExtended( in float3 color, in float maxWhite) -> float3
    {
        float lOld = Luminance(color);
        float numerator = lOld * (1.0f + (lOld / (maxWhite * maxWhite)));
        float lNew = numerator / (1.0f + lOld);
        return ChangeLuminance(color, lNew);
	}

    func ReinhardJodie( in float3 color ) -> float3 
    {
        float l = Luminance(color);
        float3 tv = color / (1.0f + color);
        return lerp( color / ( 1.0f + l), tv, tv);
    }

    func HablePartial(in float3 color) -> float3
    {
        float a = 0.15f;
        float b = 0.50f;
        float c = 0.10f;
        float d = 0.20f;
        float e = 0.02f;
        float f = 0.30f;
        return ((color * (a * color + c * b) + d * e) / (color * (a * color + b) + d * f)) - e / f;
    }

    func HableFilmic(in float3 color) -> float3
    {
        float exposure = u_Params.Exposure;
        float3 curr = HablePartial(color * exposure);

        float3 w = float3( 11.2f );
        float3 whiteScale = float3(1.0f) / HablePartial(w);

        return curr * whiteScale;
    }

	[[shader( "compute" )]]
	[[numthreads( 16, 16, 1 )]]
	func tonemapMain( uint3 dispatchThreadID : SV_DispatchThreadID ) -> void
    {
        uint2 dimensions;

        u_GBuffer.AlbedoOcclusion.GetDimensions(dimensions.x, dimensions.y);
		float2 uv = ComputeUV( dispatchThreadID, dimensions ); 
	
	}

}

