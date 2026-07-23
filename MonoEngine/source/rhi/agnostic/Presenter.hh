#pragma once
#include <common/Base.hh>
#include <common/Events.h>
#include <events/Event.hh>

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
		const EPresentationMedium Medium = MW_PRESENTATION_MEDIUM_NONE;
	};

	struct IPresentationInitialization2Info
	{
		const EPresentationMedium Medium = MW_PRESENTATION_MEDIUM_NONE;
	};


	struct IPresentationAcquisitionInfo 
	{
		const EPresentationMedium Medium = MW_PRESENTATION_MEDIUM_NONE;
	};


	struct IPresentationPresentInfo 
	{
		const EPresentationMedium Medium = MW_PRESENTATION_MEDIUM_NONE;
	};

	class IPresenter 
	{
	public:
		virtual ~IPresenter() = default;

		virtual void Init( const IPresentationInitializationInfo* pInfo ) NOEXCEPT = 0;
		virtual void Init2( const IPresentationInitialization2Info* pInfo ) NOEXCEPT = 0;
		virtual void Shutdown() NOEXCEPT = 0;

		virtual bool OnResize( SEvent& event ) = 0;

		NODISCARD virtual u32 Aquire( const IPresentationAcquisitionInfo* pInfo ) NOEXCEPT = 0;
		virtual void Present( const IPresentationPresentInfo* pInfo ) NOEXCEPT = 0;

		NODISCARD const virtual std::vector<Ref<ITexture2D>>& GetSwapchainImages() NOEXCEPT = 0;
		NODISCARD const EPresentationMedium GetMedium() const NOEXCEPT { return m_PresentationMedium; };

	protected: 
		EPresentationMedium m_PresentationMedium = MW_PRESENTATION_MEDIUM_NONE;
	};
}

