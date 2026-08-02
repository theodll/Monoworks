#pragma once
#include <Monoworks.hh>

#include <rhi/agnostic/Presenter.hh>

#ifdef MW_PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace Monoworks::RHI 
{

	class CVulkanQtPresenter : public IPresenter 
	{
	public:

	private:
#ifdef MW_PLATFORM_WINDOWS
		HANDLE m_ImageFileDescriptors[MFIF];
#elif defined(MW_PLATFORM_LINUX) || defined(MW_PLATFORM_OSX)
		int m_ImageFileDescriptors[MFIF];
#endif
	};

}