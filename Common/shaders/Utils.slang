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

    public struct MaterialEntityID
    {
        public uint EntityID;
        public uint MaterialID;
    }

    public static uint EncodeMaterialEntityIDPack(in MaterialEntityID materialEntityIDPack)
    {
        // Move materialEntityPack.EntityID into the first 19 bits of pack. Then populate the remainder of bits with materialEntityIDPack.MaterialID.
        return (materialEntityIDPack.EntityID & 0x7FFFFu) | ((materialEntityIDPack.MaterialID & 0x1FFFu) << 19);
    }

    public static MaterialEntityID DecodeMaterialEntityIDPack(in uint materialEntityIDPack) 
    {
        MaterialEntityID pack;

        // Access the first 19 bits of materialEntityIDPack aka the entity id.
        pack.EntityID = materialEntityIDPack & 0x7FFFFu;
        
        // Access the last 13 bits of materialEntityPack aka the material id.
        pack.MaterialID = (materialEntityIDPack >> 19) & 0x1FFFu;

        return pack;
    }

    public static float2 ComputeUV(in uint3 threadID, in uint2 dimensions)
    {
        return (float2(threadID.xy) + 0.5f) / dimensions;
    }



}