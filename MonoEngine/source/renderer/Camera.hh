#pragma once
#include <common/Base.hh>

namespace Monoworks 
{
	class CCamera 
	{
	public:
		MW_NOTHROW CCamera() NOEXCEPT = default;

		// Constructor for Orthographic Projections
		MW_NOTHROW CCamera( Vector4 clippingPlanesLRTB, Vector2 clippingPlanesNF ) NOEXCEPT;
		// Constructor for Perspective Projections
		// fovY in radiants
		MW_NOTHROW CCamera( float fovY, float aspect, Vector clippingPlanesNF ) NOEXCEPT;


		MW_NOTHROW void UpdateOrthographicProjection( Vector4 clippingPlanesLRTB, Vector2 clippingPlanesNF ) NOEXCEPT;
		MW_NOTHROW void UpdatePerspectiveProjection( float fovY, float aspect, Vector pClippingPlanesNF ) NOEXCEPT;

		MW_NOTHROW void UpdateViewDirection( Vector position, Vector direction, Vector up = Vector( .0f, -1.f, .0f ) ) NOEXCEPT;
		MW_NOTHROW void UpdateViewTarget( Vector position, Vector target, Vector up = Vector( .0f, -1.f, .0f ) ) NOEXCEPT;

		MW_NOTHROW void UpdateViewQuaternion( Vector position, Quaternion quaternion ) NOEXCEPT;
		MW_NOTHROW void UpdateViewEuler( Vector position, Angle euler ) NOEXCEPT;

		MW_NOTHROW const Matrix& GetCurrentProjectionMatrix() NOEXCEPT		{ return m_CurrentProjectionMatrix; };
		MW_NOTHROW const Matrix& GetCurrentViewMatrix() NOEXCEPT			{ return m_CurrentViewMatrix; }
		MW_NOTHROW const Matrix& GetCurrentViewProjectionMatrix() NOEXCEPT	{ return m_CurrentViewProjectionMatrix; }

		MW_NOTHROW const Matrix& GetPreviousProjectionMatrix() NOEXCEPT		{ return m_PreviousProjectionMatrix; };
		MW_NOTHROW const Matrix& GetPreviousViewMatrix() NOEXCEPT			{ return m_PreviousViewMatrix; }
		MW_NOTHROW const Matrix& GetPreviousViewProjectionMatrix() NOEXCEPT { return m_PreviousViewProjectionMatrix; }

		MW_NOTHROW const Vector& GetPosition() NOEXCEPT						{ return m_Position; }

	private:
		Matrix m_CurrentProjectionMatrix;
		Matrix m_CurrentViewMatrix;
		Matrix m_CurrentViewProjectionMatrix;

		Matrix m_PreviousProjectionMatrix;
		Matrix m_PreviousViewMatrix;
		Matrix m_PreviousViewProjectionMatrix;

		Vector m_Position; // TODO: replace with transform component
	};
}
