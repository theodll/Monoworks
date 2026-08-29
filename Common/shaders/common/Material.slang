module Material;

import Base;

namespace Monoworks
{
	public struct MaterialInput
    {
        public float3      AlbedoVector;
        public float3      EmissiveColor;
        public float       Metallicness;
        public float       Roughness;
        public float       EmissionFactor;
        public float       EnviromentMapRotation; // TODO: Implement
        public float       AmbientOcclusionFactor;
        public bool        UseNormalMap;

        public Texture2D   AlbedoMap;
        public Texture2D   NormalMap;
        public Texture2D   RoughnessMap;
        public Texture2D   MetallicMap;
        public Texture2D   OcclusionMap;
        public Sampler2D   MapSampler;
	}

}