#pragma once
#include <common/Base.hh>

namespace Monoworks
{
	class CCamera
	{
	public:
		 CCamera() NOEXCEPT = default;

		 CCamera( Vector4 clippingPlanesLRTB, Vector2 clippingPlanesNF ) NOEXCEPT;
		 CCamera( float fovY, float aspect, float clipPlaneNear, float clipPlaneFar ) NOEXCEPT;


		 void UpdateOrthographicProjection( Vector4 clippingPlanesLRTB, Vector2 clippingPlanesNF ) NOEXCEPT;
		 void UpdatePerspectiveProjection( float fovY, float aspect, Vector pClippingPlanesNF ) NOEXCEPT;

		 void UpdateViewDirection( Vector position, Vector direction, Vector up = Vector( .0f, -1.f, .0f ) ) NOEXCEPT;
		 void UpdateViewTarget( Vector position, Vector target, Vector up = Vector( .0f, -1.f, .0f ) ) NOEXCEPT;

		 void UpdateViewQuaternion( Vector position, Quaternion quaternion ) NOEXCEPT;
		 void UpdateViewEuler( Vector position, Angle euler ) NOEXCEPT;

		 const Matrix& GetCurrentProjectionMatrix() NOEXCEPT { return m_CurrentProjectionMatrix; };
		 const Matrix& GetCurrentViewMatrix() NOEXCEPT { return m_CurrentViewMatrix; }
		 const Matrix& GetCurrentViewProjectionMatrix() NOEXCEPT { return m_CurrentViewProjectionMatrix; }

		 const Matrix& GetPreviousProjectionMatrix() NOEXCEPT { return m_PreviousProjectionMatrix; };
		 const Matrix& GetPreviousViewMatrix() NOEXCEPT { return m_PreviousViewMatrix; }
		 const Matrix& GetPreviousViewProjectionMatrix() NOEXCEPT { return m_PreviousViewProjectionMatrix; }

		 const Vector& GetPosition() NOEXCEPT { return m_Position; }

	private:
		Matrix m_CurrentProjectionMatrix = Matrix( 1.f );
		Matrix m_CurrentViewMatrix = Matrix( 1.f );
		Matrix m_CurrentViewProjectionMatrix = Matrix( 1.f );

		Matrix m_PreviousProjectionMatrix = Matrix( 1.f );
		Matrix m_PreviousViewMatrix = Matrix( 1.f );
		Matrix m_PreviousViewProjectionMatrix = Matrix( 1.f );

		Vector m_Position = Vector( 0.f );
	};
}
