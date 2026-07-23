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
		static const EPresentationMedium Medium;
	};

	struct IPresentationInitialization2Info
	{
		static const EPresentationMedium Medium;
	};


	struct IPresentationAcquisitionInfo 
	{
		static const EPresentationMedium Medium;
	};


	struct IPresentationPresentInfo 
	{
		static const EPresentationMedium Medium;
	};

	class IPresenter 
	{
	public:
		virtual ~IPresenter() = default;

		virtual void Init( const IPresentationInitializationInfo* pInfo ) NOEXCEPT;
		virtual void Init2( const IPresentationInitialization2Info* pInfo ) NOEXCEPT;
		virtual void Shutdown() NOEXCEPT;

		virtual bool OnResize( SEvent& event );

		NODISCARD virtual u32 Aquire( const IPresentationAcquisitionInfo* pInfo ) NOEXCEPT;
		virtual void Present( const IPresentationPresentInfo* pInfo ) NOEXCEPT;

		NODISCARD const virtual std::array<ITexture2D, MaxFramesInFlight> GetSwapchainImages() NOEXCEPT = 0;
		NODISCARD const EPresentationMedium GetMedium() const NOEXCEPT { return m_PresentationMedium; };

	protected: 
		EPresentationMedium m_PresentationMedium = MW_PRESENTATION_MEDIUM_NONE;
	};
}

