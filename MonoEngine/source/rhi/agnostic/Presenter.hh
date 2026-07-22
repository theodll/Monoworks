#pragma once
#include <common/Base.hh>

#include "Texture.hh"

namespace Monoworks::RHI 
{
	enum EPresentationMedium : u8
	{
		MW_PRESENTATION_MEDIUM_NONE = 0,
		MW_PRESENTATION_MEDIUM_VULKAN_SDL, 
		MW_PRESENTATION_MEDIUM_VULKAN_QT
	};

	struct IPresentationInitializationInfo 
	{
		const EPresentationMedium Medium;
	};

	struct IPresentationAcquisitionInfo 
	{
		const EPresentationMedium Medium;
	};

	struct IPresentationPresentInfo 
	{
		const EPresentationMedium Medium;
	};

	class IPresenter 
	{
	public:
		virtual void Init( const IPresentationInitializationInfo* pInfo ) NOEXCEPT;
		virtual void Shutdown() NOEXCEPT;

		NODISCARD u32 Aquire( const IPresentationAcquisitionInfo* pInfo ) NOEXCEPT;
		void Present( const IPresentationPresentInfo* pInfo ) NOEXCEPT;

		NODISCARD const virtual std::array<ITexture2D, FramesInFlight> GetSwapchainImages() NOEXCEPT;
		NODISCARD const virtual EPresentationMedium GetMode() const NOEXCEPT;
	};
}

