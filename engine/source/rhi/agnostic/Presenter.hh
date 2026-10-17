#pragma once
#include <common/Base.hh>
#include <common/Events.hh>
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

	struct IPresentationSurfaceCreationInfo
	{
		const EPresentationMedium Medium = MW_PRESENTATION_MEDIUM_NONE;
	};

	struct IPresentationPresentInfo 
	{
		const EPresentationMedium Medium = MW_PRESENTATION_MEDIUM_NONE;
	};

	struct IPresentationTransitionPresentInfo
	{
		const EPresentationMedium Medium = MW_PRESENTATION_MEDIUM_NONE;
	};

	struct IPresentationTransitionRenderInfo
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

		virtual void CreateSurface( const IPresentationSurfaceCreationInfo* pInfo ) NOEXCEPT = 0;

		virtual bool OnResize( SEvent& event ) = 0;

		NODISCARD virtual u32 Acquire( const IPresentationAcquisitionInfo* pInfo ) NOEXCEPT = 0;
		virtual void TransitionRender( const IPresentationTransitionRenderInfo* pInfo ) NOEXCEPT = 0;
		virtual void TransitionPresent( const IPresentationTransitionPresentInfo* pInfo ) NOEXCEPT = 0;
		virtual void Present( const IPresentationPresentInfo* pInfo ) NOEXCEPT = 0;
		// TODO: rename this
		NODISCARD virtual std::vector<Ref<ITexture2D>>& GetSwapchainImages() NOEXCEPT = 0;
		NODISCARD virtual void* GetSurface() NOEXCEPT = 0;
		NODISCARD EPresentationMedium GetMedium() NOEXCEPT { return m_PresentationMedium; };

	protected: 
		EPresentationMedium m_PresentationMedium = MW_PRESENTATION_MEDIUM_NONE;
	};
}

