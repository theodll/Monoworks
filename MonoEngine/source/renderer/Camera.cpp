#include <mwpch.hh>

#include <renderer/Camera.h>

namespace Monoworks
{

	MW_NOTHROW CCamera::CCamera( Vector4 clippingPlanesLRTB, Vector2 clippingPlanesNF ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		m_CurrentProjectionMatrix = glm::ortho( clippingPlanesLRTB.x, clippingPlanesLRTB.y, clippingPlanesLRTB.z, clippingPlanesLRTB.w, clippingPlanesNF.x, clippingPlanesNF.y );
	};

	// Constructor for Perspective Projections
	MW_NOTHROW CCamera::CCamera( float fovY, float aspect, Vector clippingPlanesNF ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		m_CurrentProjectionMatrix = glm::perspective( glm::radians( fovY ), aspect, clippingPlanesNF.x, clippingPlanesNF.y );
	};

	MW_NOTHROW void CCamera::UpdateOrthographicProjection( Vector4 clippingPlanesLRTB, Vector2 clippingPlanesNF ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		m_PreviousProjectionMatrix = m_CurrentProjectionMatrix;
		m_PreviousViewProjectionMatrix = m_CurrentViewProjectionMatrix;


		m_CurrentProjectionMatrix = glm::ortho( clippingPlanesLRTB.x, clippingPlanesLRTB.y, clippingPlanesLRTB.z, clippingPlanesLRTB.w, clippingPlanesNF.x, clippingPlanesNF.y );
	};

	MW_NOTHROW void CCamera::UpdatePerspectiveProjection( float fovY, float aspect, Vector clippingPlanesNF ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		m_PreviousProjectionMatrix = m_CurrentProjectionMatrix;
		m_PreviousViewProjectionMatrix = m_CurrentViewProjectionMatrix;
			
		m_CurrentProjectionMatrix = glm::perspective( glm::radians( fovY ), aspect, clippingPlanesNF.x, clippingPlanesNF.y );
		
	};

	MW_NOTHROW void CCamera::UpdateViewDirection( Vector position, Vector direction, Vector up = Vector( .0f, -1.f, .0f ) ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		m_PreviousViewMatrix = m_CurrentViewMatrix;
		m_PreviousViewProjectionMatrix = m_CurrentViewProjectionMatrix;

		m_CurrentViewMatrix = glm::lookAt( position, direction, up );
		m_CurrentViewProjectionMatrix = m_CurrentViewMatrix * m_CurrentProjectionMatrix;
	};

	MW_NOTHROW void CCamera::UpdateViewTarget( Vector position, Vector target, Vector up = Vector( .0f, -1.f, .0f ) ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		
		UpdateViewDirection( position, target - position, up );

	};

	MW_NOTHROW void CCamera::UpdateViewQuaternion( Vector position, Quaternion quaternion ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		m_PreviousViewMatrix = m_CurrentViewMatrix;
		m_PreviousViewProjectionMatrix = m_CurrentViewProjectionMatrix;

		m_CurrentViewMatrix = glm::mat4_cast( glm::conjugate( quaternion ) ) * glm::translate( Matrix( 1.f ), -position );

		m_CurrentViewProjectionMatrix = m_CurrentViewMatrix * m_CurrentViewProjectionMatrix;
	};
	
	MW_NOTHROW void CCamera::UpdateViewEuler( Vector position, Angle euler ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;

		auto quat = Quaternion( euler );
		UpdateViewQuaternion( position, quat );

	};
}