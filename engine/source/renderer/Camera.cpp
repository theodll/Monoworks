#include <mwpch.hh>

#include <renderer/Camera.hh>

namespace Monoworks
{

	 CCamera::CCamera( Vector4 clippingPlanesLRTB, Vector2 clippingPlanesNF ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_CurrentProjectionMatrix = glm::ortho( clippingPlanesLRTB.x, clippingPlanesLRTB.y, clippingPlanesLRTB.z, clippingPlanesLRTB.w, clippingPlanesNF.x, clippingPlanesNF.y );
		m_CurrentViewProjectionMatrix = m_CurrentProjectionMatrix * m_CurrentViewMatrix;

		m_PreviousProjectionMatrix = m_CurrentProjectionMatrix;
		m_PreviousViewMatrix = m_CurrentViewMatrix;
		m_PreviousViewProjectionMatrix = m_CurrentViewProjectionMatrix;
	}

	 CCamera::CCamera( float fovY, float aspect, float clipPlaneNear, float clipPlaneFar ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_CurrentProjectionMatrix = glm::perspective( glm::radians( fovY ), aspect, clipPlaneNear, clipPlaneFar );
		m_CurrentViewProjectionMatrix = m_CurrentProjectionMatrix * m_CurrentViewMatrix;

		m_PreviousProjectionMatrix = m_CurrentProjectionMatrix;
		m_PreviousViewMatrix = m_CurrentViewMatrix;
		m_PreviousViewProjectionMatrix = m_CurrentViewProjectionMatrix;
	}

	 void CCamera::UpdateOrthographicProjection( Vector4 clippingPlanesLRTB, Vector2 clippingPlanesNF ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_PreviousProjectionMatrix = m_CurrentProjectionMatrix;
		m_PreviousViewProjectionMatrix = m_CurrentViewProjectionMatrix;

		m_CurrentProjectionMatrix = glm::ortho( clippingPlanesLRTB.x, clippingPlanesLRTB.y, clippingPlanesLRTB.z, clippingPlanesLRTB.w, clippingPlanesNF.x, clippingPlanesNF.y );
		m_CurrentViewProjectionMatrix = m_CurrentProjectionMatrix * m_CurrentViewMatrix;
	}

	 void CCamera::UpdatePerspectiveProjection( float fovY, float aspect, float clipPlaneNear, float clipPlaneFar ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_PreviousProjectionMatrix = m_CurrentProjectionMatrix;
		m_PreviousViewProjectionMatrix = m_CurrentViewProjectionMatrix;

		m_CurrentProjectionMatrix = glm::perspective( glm::radians( fovY ), aspect, clipPlaneNear, clipPlaneFar );
		m_CurrentViewProjectionMatrix = m_CurrentProjectionMatrix * m_CurrentViewMatrix;
	}

	 void CCamera::UpdateViewDirection( Vector position, Vector direction, Vector up ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_PreviousViewMatrix = m_CurrentViewMatrix;
		m_PreviousViewProjectionMatrix = m_CurrentViewProjectionMatrix;

		m_Position = position;
		m_CurrentViewMatrix = glm::lookAt( position, position + direction, up );
		m_CurrentViewProjectionMatrix = m_CurrentProjectionMatrix * m_CurrentViewMatrix;
	}

	 void CCamera::UpdateViewTarget( Vector position, Vector target, Vector up ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		UpdateViewDirection( position, target - position, up );
	}

	 void CCamera::UpdateViewQuaternion( Vector position, Quaternion quaternion ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_PreviousViewMatrix = m_CurrentViewMatrix;
		m_PreviousViewProjectionMatrix = m_CurrentViewProjectionMatrix;

		m_Position = position;
		m_CurrentViewMatrix = glm::mat4_cast( glm::conjugate( quaternion ) ) * glm::translate( Matrix( 1.f ), -position );
		m_CurrentViewProjectionMatrix = m_CurrentProjectionMatrix * m_CurrentViewMatrix;
	}

	 void CCamera::UpdateViewEuler( Vector position, Angle euler ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		UpdateViewQuaternion( position, Quaternion( euler ) );
	}
}