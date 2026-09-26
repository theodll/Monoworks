import Utils;
import GPassBase;

namespace Monoworks
{

	enum ETonemapMode 
	{
		MW_TONEMAP_MODE_NONE, // Pass through
		MW_TONEMAP_MODE_REINHARD, 
		MW_TONEMAP_MODE_REINHARD_EXTENDED,
        MW_TONEMAP_MODE_REINHARD_JODIE,
        MW_TONEMAP_MODE_HABLE_FILMIC,
		MW_TONEMAP_MODE_ACES_FILMIC_APROX,
		MW_TONEMAP_MODE_AGX_APROX,
		MW_TONEMAP_MODE_KHRONOS_PBR_NEUTRAL
	}

    enum EAGXMode
    {
        MW_TONEMAP_AGX_MODE_DEFAULT,
        MW_TONEMAP_AGX_MODE_GOLDEN,
        MW_TONEMAP_AGX_MODE_PUNCHY
    }

	public struct TonemapParams
    {
        ETonemapMode Mode;
        EAGXMode AGXMode;
        float Exposure;
        bool EnableGamma;
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

        if (luminanceIn <= 0.0f)
            return float3(0.0f);

        return colorIn * (luminanceOut / luminanceIn);
    }

    func Reinhard( in float3 color) -> float3
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
        float3 curr = HablePartial(color);

        float3 w = float3( 11.2f );
        float3 whiteScale = float3(1.0f) / HablePartial(w);

        return curr * whiteScale;
    }

    func Linear(in float3 color) -> float3
    {
        return color;
    }

    func AcesApprox(float3 color) -> float3
    {
        color *= 0.6f;
        float a = 2.51f;
        float b = 0.03f;
        float c = 2.43f;
        float d = 0.59f;
        float e = 0.14f;
        return clamp( ( color * ( a * color + b ) ) / ( color * ( c * color + d ) + e), 0.0f, 1.0f);
    }

    // Credit to: https://github.com/KhronosGroup/ToneMapping/blob/main/PBR_Neutral/pbrNeutral.glsl
    func KhoronosPBRNeutral( in float3 color) -> float3
    {
        const float startCompression = 0.8 - 0.04;
        const float desaturation = 0.15;

        float x = min(color.r, min(color.g, color.b));
        float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
        color -= offset;

        float peak = max(color.r, max(color.g, color.b));
        if (peak < startCompression) return color;

        const float d = 1. - startCompression;
        float newPeak = 1. - d * d / (peak + d - startCompression);
        color *= newPeak / peak;

        float g = 1. - 1. / (desaturation * (peak - newPeak) + 1.);
        return lerp(color, newPeak * float3(1, 1, 1), g);
    }

    // Credit to https://www.shadertoy.com/view/cd3XWr
    func AGXDefaultContrastApprox(float3 x) -> float3 
    {
        float3 x2 = x * x;
        float3 x4 = x2 * x2;

        return +15.5 * x4 * x2
         - 40.14 * x4 * x
         + 31.96 * x4
         - 6.868 * x2 * x
         + 0.4298 * x2
         + 0.1191 * x
         - 0.00232;
    }

    func AGX(float3 val) -> float3
    {
        const float3x3 agxMat = float3x3(
    0.842479062253094, 0.0423282422610123, 0.0423756549057051,
    0.0784335999999992, 0.878468636469772, 0.0784336,
    0.0792237451477643, 0.0791661274605434, 0.879142973793104);

        const float minEv = -12.47393f;
        const float maxEv = 4.026069f;

        // Input transform
        val = mul(agxMat, val);

        // Log2 space encoding
        val = clamp(log2(val), minEv, maxEv);
        val = (val - minEv) / (maxEv - minEv);

        // Apply sigmoid function approximation
        val = AGXDefaultContrastApprox(val);

        return val;
    }

    func AGXEotf(float3 val) -> float3 {
        const float3x3 agxMatInv = float3x3(
    1.19687900512017, -0.0528968517574562, -0.0529716355144438,
    -0.0980208811401368, 1.15190312990417, -0.0980434501171241,
    -0.0990297440797205, -0.0989611768448433, 1.15107367264116);

        // Undo input transform
        val = mul( agxMatInv, val );

        // sRGB IEC 61966-2-1 2.2 Exponent Reference EOTF Display
        val = pow(val, float3(2.2));

        return val;
    }

    func AGXLook(float3 val) -> float3 
    {
        // Default
        float3 offset = float3(0.0);
        float3 slope =  float3(1.0);
        float3 power =  float3(1.0);
        float sat = 1.0;

        if (u_Params.AGXMode == EAGXMode.MW_TONEMAP_AGX_MODE_GOLDEN)
        {
        // Golden
        slope = float3(1.0, 0.9, 0.5);
        power = float3(0.8);
        sat = 0.8;
        }
        else if (u_Params.AGXMode == EAGXMode.MW_TONEMAP_AGX_MODE_PUNCHY)
        {
        // Punchy
        slope = float3(1.0);
        power = float3(1.35, 1.35, 1.35);
        sat = 1.4;
        }

        // ASC CDL
        val = pow(val * slope + offset, power);

        const float3 lw = float3(0.2126, 0.7152, 0.0722);
        float luma = dot(val, lw);

        return luma + sat * (val - luma);
    }

    [[shader( "compute" )]]
	[[numthreads( 16, 16, 1 )]]
	func tonemapMain( uint3 dispatchThreadID : SV_DispatchThreadID ) -> void
    {
        uint2 dimensions;
        u_GBuffer.AlbedoOcclusion.GetDimensions(dimensions.x, dimensions.y);

        if (dispatchThreadID.x >= dimensions.x || dispatchThreadID.y >= dimensions.y)
            return;

        float4 hdrSample = u_GBuffer.Composite.Load(int2(dispatchThreadID.xy));

        float3 color = hdrSample.rgb * u_Params.Exposure;

        switch (u_Params.Mode)
        {
        case ETonemapMode.MW_TONEMAP_MODE_NONE:
            color = Linear(color);
            break;
        case ETonemapMode.MW_TONEMAP_MODE_REINHARD:
            color = Reinhard(color);
            break;
        case ETonemapMode.MW_TONEMAP_MODE_REINHARD_EXTENDED:
        // TODO: dont make this static
            color = ReinhardExtended(color, 16.0f);
            break;
        case ETonemapMode.MW_TONEMAP_MODE_REINHARD_JODIE:
            color = ReinhardJodie(color);
            break;
        case ETonemapMode.MW_TONEMAP_MODE_HABLE_FILMIC:
            color = HableFilmic(color);
            break;
        case ETonemapMode.MW_TONEMAP_MODE_ACES_FILMIC_APROX:
            color = AcesApprox(color);
            break;
        case ETonemapMode.MW_TONEMAP_MODE_AGX_APROX:
            color = AGX(color);
            color = AGXLook(color);
            color = AGXEotf( color );
            break;
        case ETonemapMode.MW_TONEMAP_MODE_KHRONOS_PBR_NEUTRAL:
            color = KhoronosPBRNeutral(color);
            break;
        default:
            break;
        }

        if (u_Params.EnableGamma)
        {
            color = pow( color, float3(1.0f / 2.2f));
        }

        u_SwapchainImage[dispatchThreadID.xy] = float4(color, hdrSample.a);

	}

}

