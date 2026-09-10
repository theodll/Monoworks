module Utils;

namespace Monoworks 
{

    /// @brief Encodes a three-component unit vector into a two-component packed vector.    
	public static float2 OctEncode(in float3 inf)
    {
        float3 n = inf;
        n /= (abs(n.x) + abs(n.y) + abs(n.z));
        float2 oct = n.xy;
        if (n.z < 0.0)
            oct = (1.0 - abs(oct.yx)) * (oct.xy >= 0.0 ? 1.0 : -1.0);
        return oct * 0.5 + 0.5;
    };

    /// @brief Encodes a three-component unit vector into a two-component packed vector.    
    public static float3 OctDecode(in float2 inf)
    {
        float2 f = inf;
        f = f * 2.0 - 1.0;
        float3 n = float3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
        float t = saturate(-n.z);
        n.xy += n.xy >= 0.0 ? -t : t;
        return normalize(n);
    }


}