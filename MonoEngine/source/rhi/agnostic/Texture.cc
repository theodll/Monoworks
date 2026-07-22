#include <mwpch.hh>
#include <rhi/Types.hh>

#include <core/Application.hh>

#include <rhi/specific/vulkan/VulkanTexture.hh>

#include "Texture.hh"

namespace Monoworks::RHI
{
	NODISCARD static Ref<ITexture2D> Create(const path_t* pPath) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		switch (CApplication::GetGraphicsAPI())
		{
		case MW_GAPI_NONE:    return nullptr;
		case MW_GAPI_VULKAN:  return Ref<CVulkanTexture2D>::Create(pPath);
		}
		MW_ASSERT(false, "Unknown Graphics API");
		return nullptr;
	};

	NODISCARD static Ref<ITexture2D> Create(s32 width, s32 height) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		switch (CApplication::GetGraphicsAPI())
		{
		case MW_GAPI_NONE:    return nullptr;
		case MW_GAPI_VULKAN:  return Ref<CVulkanTexture2D>::Create(width, height);
		}
		MW_ASSERT(false, "Unknown Graphics API");
		return nullptr;
	};
	
	NODISCARD static Ref<ITexture2D> Create(const STextureCreateInfo* pInfo) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		switch (CApplication::GetGraphicsAPI())
		{
		case MW_GAPI_NONE:    return nullptr;
		case MW_GAPI_VULKAN:  return Ref<CVulkanTexture2D>::Create(pInfo);
		}
		MW_ASSERT(false, "Unknown Graphics API");
		return nullptr;
	};
}

