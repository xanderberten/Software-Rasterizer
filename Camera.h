#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <algorithm>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle) :
			origin{ _origin },
			fovAngle{ _fovAngle }
		{
		}

		Vector3 origin{};
		float fovAngle{ 90.f };
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };
		float aspectRatio{};

		//These ones are used to check weather or not the value has changed
		float oldFOV{0.f};
		float oldAspectRatio{0.f};

		Vector3 forward{ Vector3::UnitZ };
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};

		Matrix projectionMatrix{};
		
		const float nearPlane{ .1f };
		const float farPlane{ 500.f };


		void Initialize(float _fovAngle = 90.f, Vector3 _origin = { 0.f,0.f,0.f }, float _aspectRatio = 0.f)
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);
			origin = _origin;
			aspectRatio = _aspectRatio;
		}

		void CalculateViewMatrix()
		{
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right);

			invViewMatrix = Matrix
			{
				right,
				up,
				forward,
				origin
			};

			viewMatrix = invViewMatrix.Inverse();

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			projectionMatrix = Matrix::CreatePerspectiveFovLH(fov,aspectRatio,nearPlane,farPlane);	

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(const Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			// Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			// Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			constexpr float keyboardSpeed{ 8.f };
			constexpr float mouseSpeed{ 0.7f };
			constexpr float angularSpeed{ 5.0f * TO_RADIANS };

			Vector3 direction{};
			direction += (pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_Z]) * forward * keyboardSpeed * deltaTime;
			direction -= pKeyboardState[SDL_SCANCODE_S] * forward * keyboardSpeed * deltaTime;
			direction -= (pKeyboardState[SDL_SCANCODE_Q] || pKeyboardState[SDL_SCANCODE_A]) * right * keyboardSpeed * deltaTime;
			direction += pKeyboardState[SDL_SCANCODE_D] * right * keyboardSpeed * deltaTime;

			switch (mouseState)
			{
			case SDL_BUTTON_LMASK:
				direction -= forward * (mouseY * mouseSpeed * deltaTime);
				totalYaw += mouseX * angularSpeed * deltaTime;
				break;
			case SDL_BUTTON_RMASK:
				totalYaw += mouseX * angularSpeed * deltaTime;
				totalPitch -= mouseY * angularSpeed * deltaTime;
				break;
			case SDL_BUTTON_X2: 
				direction.y -= mouseY * mouseSpeed * deltaTime;
				break;
			}
			totalPitch = std::clamp(totalPitch, -90.f * TO_RADIANS, 90.f * TO_RADIANS);

			origin += direction;


			const Matrix rotationMatrix = Matrix::CreateRotationX(totalPitch) * Matrix::CreateRotationY(totalYaw);
			forward = rotationMatrix.TransformVector(Vector3::UnitZ);

			//Update Matrices
			CalculateViewMatrix();

			if(DidFovOrAspectRatioChange())
			{
				CalculateProjectionMatrix();
			}
		}

		bool DidFovOrAspectRatioChange()
		{
			//This will initialize the values that are needed in the future
			if (oldAspectRatio == 0.f || oldFOV == 0.f)
			{
				oldAspectRatio = aspectRatio;
				oldFOV = fov;
				CalculateProjectionMatrix();
			}

			if(oldAspectRatio != aspectRatio || oldFOV != fov)
			{
				return true;
			}

		return false;
		}

		bool IsOutsideFrustum(const Vector4& vector)const
		{
			return vector.x < -1.f || vector.x > 1.f
				|| vector.y < -1.f || vector.y > 1.f
				|| vector.z < -1.f || vector.z > 1.f;
		}
	};
}