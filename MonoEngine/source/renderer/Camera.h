#pragma once
#include <common/Base.hh>

namespace Monoworks 
{
	class CCamera 
	{
	public:
		MW_NOTHROW CCamera( const Matrix* pProjection ) NOEXCEPT;

		MW_NOTHROW void UpdateOrthographicProjection( Vector4 pClippingPlanesLRTB, Vector2 pClippingPlanesNF ) NOEXCEPT;
		MW_NOTHROW void UpdatePerspectiveProjection( float fovY, float aspect, Vector pClippingPlanesNF ) NOEXCEPT;

		MW_NOTHROW void UpdateViewDirection( Vector position, Vector direction, Vector up = Vector( .0f, -1.f, .0f ) ) NOEXCEPT;
		MW_NOTHROW void UpdateViewTarget( Vector position, Vector target, Vector up = Vector( .0f, -1.f, .0f ) ) NOEXCEPT;

		MW_NOTHROW void UpdateViewQuaternion( Vector position, Quaternion quaternion ) NOEXCEPT;
		MW_NOTHROW void UpdateViewEuler( Vector position, Angle euler ) NOEXCEPT;



	private:
		Matrix m_CurrentProjectionMatrix;
		Matrix m_CurrentViewMatrix;
		Matrix m_CurrentViewProjectionMatrix;

		Matrix m_PreviousProjectionMatrix;
		Matrix m_PreviousViewMatrix;
		Matrix m_PreviousViewProjectionMatrix;

		Vector m_Position;
	};
}
