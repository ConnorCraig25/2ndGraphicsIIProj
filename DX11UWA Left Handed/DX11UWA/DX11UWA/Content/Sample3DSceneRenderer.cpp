#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"

using namespace DX11UWA;

using namespace DirectX;
using namespace Windows::Foundation;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_degreesPerSecond(45),
	m_indexCount(0),
	m_tracking(false),
	m_deviceResources(deviceResources)
{
	memset(m_kbuttons, 0, sizeof(m_kbuttons));
	m_currMousePos = nullptr;
	m_prevMousePos = nullptr;
	memset(&m_camera, 0, sizeof(XMFLOAT4X4));

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources(void)
{
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 0.01f, 100.0f);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(&m_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const XMVECTORF32 eye = { 0.0f, 0.7f, -1.5f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	XMStoreFloat4x4(&m_camera, XMMatrixInverse(nullptr, XMMatrixLookAtLH(eye, at, up)));
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	if (!m_tracking)
	{
		// Convert degrees to radians, then convert seconds to rotation angle
		float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
		double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
		float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

		Rotate(0);
	}


	// Update or move camera here
	UpdateCamera(timer, 10.0f, 0.75f);

	XMStoreFloat4(&m_LightProperties.EyePosition, XMVectorSet(m_camera._41, m_camera._42, m_camera._43, 1.0f));

	m_skyBoxBufferData.view = m_constantBufferData.view;
	m_skyBoxBufferData.projection = m_constantBufferData.projection;
	XMStoreFloat4x4(&m_skyBoxBufferData.model, XMMatrixTranspose(XMMatrixTranslation(m_camera._41, m_camera._42, m_camera._43)));

	for (int i = 0; i < numLights; ++i)
	{
		Light light;
		XMFLOAT4 LightPosition;
		memset(&light, 0, sizeof(Light));
		light.LightTypeEnabled.y = LightEnabled[i];
		light.LightTypeEnabled.x = i;
		light.Color = XMFLOAT4(LightColors[i]);
		light.AttenuationData.x = XMConvertToRadians(45.0f);
		light.AttenuationData.y = 1.0f;
		light.AttenuationData.z = 2.0f;
		light.AttenuationData.w = 0.0f;

		//Directional light location
		if (i == 0)
		{
			if (dirLightSwitch)
			{
				directionalLightPos.x += .5;
				if (directionalLightPos.x >= 15)
				{
					dirLightSwitch = false;
				}
			}
			else
			{
				directionalLightPos.x -= .5;
				if (directionalLightPos.x <= -15)
				{
					dirLightSwitch = true;
				}
			}

			LightPosition = directionalLightPos;
		}

		//point light location
		if (i == 1)
		{
			if (pointLightSwitch)
			{
				pointLightPos.x += .2f;
				if (pointLightPos.x >= 10)
				{
					pointLightSwitch = false;
				}
			}
			else
			{
				pointLightPos.x -= .2f;
				if (pointLightPos.x <= -10)
				{
					pointLightSwitch = true;
				}
			}
			LightPosition = pointLightPos;
		}

		//Spot Light Information
		if (i == 2) // Spot light Location
		{
			if (spotLightSwitch)
			{
				spotPos.z += .1f;
				XMStoreFloat4(&light.coneAngle, XMVectorSet(coneAng.x, coneAng.y, coneAng.z + 1.0f, 0.0f));
				if (spotPos.z >= 2)
				{
					spotLightSwitch = false;
				}
			}
			else
			{
				spotPos.z -= .1f;
				XMStoreFloat4(&light.coneAngle, XMVectorSet(coneAng.x, coneAng.y, coneAng.z - 1.0f, 0.0f));
				if (spotPos.z <= -2)
				{
					spotLightSwitch = true;
				}
			}
			LightPosition = spotPos;
		}

		light.radius.x = spotRad;
		light.ConeRatio.x = innerConeRat;
		light.ConeRatio.y = outterConeRat;
		
		light.Position = LightPosition;
		XMVECTOR LightDirection = XMVectorSet(-LightPosition.x, -LightPosition.y, -LightPosition.z, 0.0f);
		LightDirection = XMVector3Normalize(LightDirection);
		XMStoreFloat4(&light.Direction, LightDirection);

		m_LightProperties.Lights[i] = light;

	}

	//m_d3dDeviceContext->UpdateSubresource(m_d3dLightPropertiesConstantBuffer.Get(), 0, nullptr, &m_LightProperties, 0, 0);
	m_deviceResources->GetD3DDeviceContext()->UpdateSubresource(lightbuffer.Get(), 0, NULL, &m_LightProperties, 0, 0);

	for (unsigned int i = 0; i < 3; i++)
	{
		m_constBufferPyramidData.model[i] = m_constantBufferData.model;
		m_constBufferPyramidData.projection = m_constantBufferData.projection;
		m_constBufferPyramidData.view = m_constantBufferData.view;
		XMStoreFloat4x4(&m_constBufferPyramidData.model[i],
			XMMatrixTranspose(XMMatrixTranslation(m_LightProperties.Lights[i].Position.x,
												  m_LightProperties.Lights[i].Position.y,
												  m_LightProperties.Lights[i].Position.z)));
	}

	//CreateDDSTextureFromMemory(m_deviceResources->GetD3DDevice(),)


		//CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(),
		//	L"Assets/pokeball.dds",
		//	nullptr,
		//	m_pokeplatTex.GetAddressOf());
}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
	// Prepare to pass the updated model matrix to the shader
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
}

void Sample3DSceneRenderer::UpdateCamera(DX::StepTimer const& timer, float const moveSpd, float const rotSpd)
{
	const float delta_time = (float)timer.GetElapsedSeconds();

	if (m_kbuttons['W'])
	{
		XMMATRIX translation = XMMatrixTranslation(0.0f, 0.0f, moveSpd * delta_time);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['S'])
	{
		XMMATRIX translation = XMMatrixTranslation(0.0f, 0.0f, -moveSpd * delta_time);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['A'])
	{
		XMMATRIX translation = XMMatrixTranslation(-moveSpd * delta_time, 0.0f, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['D'])
	{
		XMMATRIX translation = XMMatrixTranslation(moveSpd * delta_time, 0.0f, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['X'])
	{
		XMMATRIX translation = XMMatrixTranslation( 0.0f, -moveSpd * delta_time, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons[VK_SPACE])
	{
		XMMATRIX translation = XMMatrixTranslation( 0.0f, moveSpd * delta_time, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}

	if (m_currMousePos) 
	{
		if (m_currMousePos->Properties->IsRightButtonPressed && m_prevMousePos)
		{
			float dx = m_currMousePos->Position.X - m_prevMousePos->Position.X;
			float dy = m_currMousePos->Position.Y - m_prevMousePos->Position.Y;

			XMFLOAT4 pos = XMFLOAT4(m_camera._41, m_camera._42, m_camera._43, m_camera._44);

			m_camera._41 = 0;
			m_camera._42 = 0;
			m_camera._43 = 0;

			XMMATRIX rotX = XMMatrixRotationX(dy * rotSpd * delta_time);
			XMMATRIX rotY = XMMatrixRotationY(dx * rotSpd * delta_time);

			XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
			temp_camera = XMMatrixMultiply(rotX, temp_camera);
			temp_camera = XMMatrixMultiply(temp_camera, rotY);

			XMStoreFloat4x4(&m_camera, temp_camera);

			m_camera._41 = pos.x;
			m_camera._42 = pos.y;
			m_camera._43 = pos.z;
		}
		m_prevMousePos = m_currMousePos;
	}


}

void Sample3DSceneRenderer::SetKeyboardButtons(const char* list)
{
	memcpy_s(m_kbuttons, sizeof(m_kbuttons), list, sizeof(m_kbuttons));
}

void Sample3DSceneRenderer::SetMousePosition(const Windows::UI::Input::PointerPoint^ pos)
{
	m_currMousePos = const_cast<Windows::UI::Input::PointerPoint^>(pos);
}

void Sample3DSceneRenderer::SetInputDeviceData(const char* kb, const Windows::UI::Input::PointerPoint^ pos)
{
	SetKeyboardButtons(kb);
	SetMousePosition(pos);
}

void DX11UWA::Sample3DSceneRenderer::StartTracking(void)
{
	m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void Sample3DSceneRenderer::TrackingUpdate(float positionX)
{
	if (m_tracking)
	{
		float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
		Rotate(radians);
	}
}

void Sample3DSceneRenderer::StopTracking(void)
{
	m_tracking = false;
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render(void)
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));

	
	//// Prepare the constant buffer to send it to the graphics device.
	//context->UpdateSubresource1(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);
	//// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionUVNormal);
	 const UINT offset = 0;

//context->UpdateSubresource1(m_constantBuffer.Get(), 0, NULL, &m_skyBoxBufferData, 0, 0, 0);
//	context->IASetVertexBuffers(0, 1, m_VertSkyboxBuffer.GetAddressOf(), &stride, &offset);
//	context->IASetIndexBuffer(m_IndexSkyboxBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
//	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
//	context->IASetInputLayout(m_inputLayout.Get());
//	// Attach our vertex shader.
//	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
//	context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
//	context->HSSetShader(m_hulShader.Get(), nullptr, 0);
//	context->DSSetShader(m_domShader.Get(), nullptr, 0);
//	context->DSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
//	context->GSSetShader(m_geoShader.Get(), nullptr, 0);
//	// Send the constant buffer to the graphics device.
//
//	// Attach our pixel shader.
//	context->PSSetShader(m_pyramid_pixelShader.Get(), nullptr, 0);
//	context->PSSetShaderResources(0, 1, m_SkyboxTex.GetAddressOf());
//	// Draw the objects.
//	context->DrawIndexed(m_indexSkyboxCount, 0, 0);
//	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
//	
	 	context->UpdateSubresource1(m_constPyramidBuffer.Get(), 0, NULL, &m_constBufferPyramidData, 0, 0, 0);
		context->IASetVertexBuffers(0, 1, m_VertPyramidBuffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(m_IndexPyramidBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		context->IASetInputLayout(m_inputLayout.Get());
		context->VSSetShader(m_instancedvertexShader.Get(), nullptr, 0);
		context->VSSetConstantBuffers1(0, 1, m_constPyramidBuffer.GetAddressOf(), nullptr, nullptr);
		context->DSSetShader(nullptr, nullptr, 0);
		context->HSSetShader(nullptr, nullptr, 0);
		context->GSSetShader(nullptr, nullptr, 0);
		context->PSSetShader(m_pyramid_pixelShader.Get(), nullptr, 0);
		
		// Draw the objects.
		context->DrawIndexedInstanced(m_indexPyramidCount, 3, 0, 0, 0);





	
	
	stride = sizeof(VertexPositionUVNormal);
	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);

	context->PSSetConstantBuffers(0, 1, lightbuffer.GetAddressOf());
	context->IASetVertexBuffers(0, 1, m_Vertfloor_bottomBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(m_Indexfloor_bottomBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	context->IASetInputLayout(m_inputLayout.Get());
	// Attach our vertex shader.
	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->HSSetShader(m_hulShader.Get(), nullptr, 0);
	
	context->DSSetShader(m_domShader.Get(), nullptr, 0);
	context->DSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->GSSetShader(nullptr, nullptr, 0);
	// Send the constant buffer to the graphics device.
	
	// Attach our pixel shader.
	context->PSSetShader(m_light_pixelShader.Get(), nullptr, 0);
	context->PSSetShaderResources(0, 1, m_floor_bottomTex.GetAddressOf());
	// Draw the objects.
	context->DrawIndexed(m_indexfloor_bottomCount, 0, 0);




	stride = sizeof(VertexPositionUVNormal);
	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);

	context->PSSetConstantBuffers(0, 1, lightbuffer.GetAddressOf());
	context->IASetVertexBuffers(0, 1, m_Vertfloor_platformBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(m_Indexfloor_platformBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	context->IASetInputLayout(m_inputLayout.Get());
	// Attach our vertex shader.
	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->HSSetShader(m_hulShader.Get(), nullptr, 0);
	context->DSSetShader(m_domShader.Get(), nullptr, 0);
	context->DSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->GSSetShader(nullptr, nullptr, 0);
	// Send the constant buffer to the graphics device.

	// Attach our pixel shader.
	context->PSSetShader(m_light_pixelShader.Get(), nullptr, 0);
	// Draw the objects.
	context->DrawIndexed(m_indexfloor_platformCount, 0, 0);


	stride = sizeof(VertexPositionUVNormal);
	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);

	context->PSSetConstantBuffers(0, 1, lightbuffer.GetAddressOf());
	context->IASetVertexBuffers(0, 1, m_Vertpokeplat_redBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(m_Indexpokeplat_redBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	context->IASetInputLayout(m_inputLayout.Get());
	// Attach our vertex shader.
	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->HSSetShader(m_hulShader.Get(), nullptr, 0);
	context->DSSetShader(m_domShader.Get(), nullptr, 0);
	context->DSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->GSSetShader(nullptr, nullptr, 0);
	// Send the constant buffer to the graphics device.

	// Attach our pixel shader.
	context->PSSetShader(m_light_pixelShader.Get(), nullptr, 0);
	context->PSSetShaderResources(0, 1, m_pokeplatTex.GetAddressOf());
	// Draw the objects.
	context->DrawIndexed(m_indexpokeplat_redCount, 0, 0);

	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);

	context->PSSetConstantBuffers(0, 1, lightbuffer.GetAddressOf());
	context->IASetVertexBuffers(0, 1, m_Vertpokeplat_whiteBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(m_Indexpokeplat_whiteBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	context->IASetInputLayout(m_inputLayout.Get());
	// Attach our vertex shader.
	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->HSSetShader(m_hulShader.Get(), nullptr, 0);
	context->DSSetShader(m_domShader.Get(), nullptr, 0);
	context->DSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->GSSetShader(nullptr, nullptr, 0);
	// Send the constant buffer to the graphics device.

	// Attach our pixel shader.
	context->PSSetShader(m_light_pixelShader.Get(), nullptr, 0);
	context->PSSetShaderResources(0, 1, m_pokeplatTex.GetAddressOf());
	// Draw the objects.
	context->DrawIndexed(m_indexpokeplat_whiteCount, 0, 0);

	context->PSSetConstantBuffers(0, 1, lightbuffer.GetAddressOf());
	context->IASetVertexBuffers(0, 1, m_Vertpokeplat_blackBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(m_Indexpokeplat_blackBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	context->IASetInputLayout(m_inputLayout.Get());
	// Attach our vertex shader.
	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->HSSetShader(m_hulShader.Get(), nullptr, 0);
	context->DSSetShader(m_domShader.Get(), nullptr, 0);
	context->DSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->GSSetShader(nullptr, nullptr, 0);
	// Send the constant buffer to the graphics device.

	// Attach our pixel shader.
	context->PSSetShader(m_light_pixelShader.Get(), nullptr, 0);
	context->PSSetShaderResources(0, 1, m_pokeplatTex.GetAddressOf());
	// Draw the objects.
	context->DrawIndexed(m_indexpokeplat_blackCount, 0, 0);

	context->PSSetConstantBuffers(0, 1, lightbuffer.GetAddressOf());
	context->IASetVertexBuffers(0, 1, m_VertstadiumBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(m_IndexstadiumBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	context->IASetInputLayout(m_inputLayout.Get());
	// Attach our vertex shader.
	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->HSSetShader(m_hulShader.Get(), nullptr, 0);
	context->DSSetShader(m_domShader.Get(), nullptr, 0);
	context->DSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->GSSetShader(nullptr, nullptr, 0);
	// Send the constant buffer to the graphics device.

	// Attach our pixel shader.
	context->PSSetShader(m_light_pixelShader.Get(), nullptr, 0);
	context->PSSetShaderResources(0, 1, m_pokeplatTex.GetAddressOf());
	// Draw the objects.
	context->DrawIndexed(m_indexstadiumCount, 0, 0);

	context->PSSetConstantBuffers(0, 1, lightbuffer.GetAddressOf());
	context->IASetVertexBuffers(0, 1, m_Vertstadium_topBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(m_Indexstadium_topBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	context->IASetInputLayout(m_inputLayout.Get());
	// Attach our vertex shader.
	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->HSSetShader(m_hulShader.Get(), nullptr, 0);
	context->DSSetShader(m_domShader.Get(), nullptr, 0);
	context->DSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->GSSetShader(m_geoShader.Get(), nullptr, 0);
	// Send the constant buffer to the graphics device.

	// Attach our pixel shader.
	context->PSSetShader(m_pyramid_pixelShader.Get(), nullptr, 0);
	context->PSSetShaderResources(0, 1, m_pokeplatTex.GetAddressOf());
	// Draw the objects.
	context->DrawIndexed(m_indexstadium_topCount, 0, 0);


	

}

void Sample3DSceneRenderer::CreateDeviceDependentResources(void)
{

	CD3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(CD3D11_SAMPLER_DESC));

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.MipLODBias = 0.0f;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.BorderColor[0] = 1.0f;
	sampDesc.BorderColor[1] = 1.0f;
	sampDesc.BorderColor[2] = 1.0f;
	sampDesc.BorderColor[3] = 1.0f;
	sampDesc.MinLOD = -FLT_MAX;
	sampDesc.MaxLOD = FLT_MAX;

	// Load shaders asynchronously.
	auto loadVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso");
	auto loadInstanceVStask = DX::ReadDataAsync(L"InstancedVertexShader.cso");
	auto loadDSTask = DX::ReadDataAsync(L"DomainShader.cso");
	//auto loadInstanceDSTask = DX::ReadDataAsync(L"InstancedDomainShader.cso");
	auto loadHSTasK = DX::ReadDataAsync(L"HullShader.cso");
	auto loadPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso");
	auto loadLightPSTask = DX::ReadDataAsync(L"LightPixelShader.cso");
	auto loadPyramidPSTask = DX::ReadDataAsync(L"PyramidPixelShader.cso");
	auto loadGSTask = DX::ReadDataAsync(L"GeometryShader.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateVertexShader(&fileData[0], fileData.size(), nullptr, &m_vertexShader));
		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "UV", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), &fileData[0], fileData.size(), &m_inputLayout));
	});
	auto createInstanceVSTask = loadInstanceVStask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateVertexShader(&fileData[0], fileData.size(), nullptr, &m_instancedvertexShader));
	});
	auto createHSTask = loadHSTasK.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateHullShader(&fileData[0], fileData.size(), nullptr, &m_hulShader));
	});
	auto createDSTask = loadDSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateDomainShader(&fileData[0], fileData.size(), nullptr, &m_domShader));
		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, &m_constantBuffer));
	});
	auto createGSTask = loadGSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateGeometryShader(&fileData[0],fileData.size(),nullptr, &m_geoShader));
		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(m_geoBuffer), D3D11_BIND_STREAM_OUTPUT);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, &m_geoBuffer));
	});
	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreatePixelShader(&fileData[0], fileData.size(), nullptr, &m_pixelShader));

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, &m_constantBuffer));
	});
	auto createPyramidPSTask = loadPyramidPSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreatePixelShader(&fileData[0], fileData.size(), nullptr, &m_pyramid_pixelShader));
		CD3D11_BUFFER_DESC instancedconstantBufferDesc(sizeof(ModelViewProjectionConstantBufferInstanced), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&instancedconstantBufferDesc, nullptr, &m_constPyramidBuffer));
	});
	auto createlightPSTask = loadLightPSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreatePixelShader(&fileData[0], fileData.size(), nullptr, &m_light_pixelShader));
		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(LightProperties), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, &lightbuffer));
	});
	// Once both shaders are loaded, create the mesh.

	auto createGroundTask = (createlightPSTask && createVSTask && createHSTask && createDSTask).then([this]()
	{
		Mesh sphere = Mesh("Assets/floor_bottom.obj");

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = sphere.uniqueVertList.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionUVNormal)*sphere.uniqueVertList.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_Vertfloor_bottomBuffer));

		m_indexfloor_bottomCount = sphere.indexbuffer.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = sphere.indexbuffer.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned int)*sphere.indexbuffer.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_Indexfloor_bottomBuffer));


		CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(),
			L"Assets/Castle1.dds",
			nullptr,
			m_floor_bottomTex.GetAddressOf());
	});

	auto createPlatformTask = (createlightPSTask && createVSTask && createHSTask && createDSTask).then([this]()
	{
		Mesh sphere = Mesh("Assets/floor_platform.obj");

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = sphere.uniqueVertList.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionUVNormal)*sphere.uniqueVertList.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_Vertfloor_platformBuffer));

		m_indexfloor_platformCount = sphere.indexbuffer.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = sphere.indexbuffer.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned int)*sphere.indexbuffer.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_Indexfloor_platformBuffer));
	});

	auto createpokeplat_redTask = (createlightPSTask && createVSTask && createHSTask && createDSTask).then([this]()
	{
		Mesh sphere = Mesh("Assets/pokeballred.obj");

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = sphere.uniqueVertList.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionUVNormal)*sphere.uniqueVertList.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_Vertpokeplat_redBuffer));

		m_indexpokeplat_redCount = sphere.indexbuffer.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = sphere.indexbuffer.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned int)*sphere.indexbuffer.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_Indexpokeplat_redBuffer));


		CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(),
			L"Assets/pokeball.dds",
			nullptr,
			m_pokeplatTex.GetAddressOf());
	});

	auto createpokeplat_whiteTask = (createlightPSTask && createVSTask && createHSTask && createDSTask).then([this]()
	{
		Mesh sphere = Mesh("Assets/pokeballwhite.obj");

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = sphere.uniqueVertList.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionUVNormal)*sphere.uniqueVertList.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_Vertpokeplat_whiteBuffer));

		m_indexpokeplat_whiteCount = sphere.indexbuffer.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = sphere.indexbuffer.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned int)*sphere.indexbuffer.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_Indexpokeplat_whiteBuffer));
	});

	auto createpokeplat_blackTask = (createlightPSTask && createVSTask && createHSTask && createDSTask).then([this]()
	{
		Mesh sphere = Mesh("Assets/pokeballblack.obj");

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = sphere.uniqueVertList.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionUVNormal)*sphere.uniqueVertList.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_Vertpokeplat_blackBuffer));

		m_indexpokeplat_blackCount = sphere.indexbuffer.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = sphere.indexbuffer.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned int)*sphere.indexbuffer.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_Indexpokeplat_blackBuffer));
	});

	auto createstadiumTask = (createlightPSTask && createVSTask && createHSTask && createDSTask).then([this]()
	{
		Mesh sphere = Mesh("Assets/stadium.obj");

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = sphere.uniqueVertList.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionUVNormal)*sphere.uniqueVertList.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_VertstadiumBuffer));

		m_indexstadiumCount = sphere.indexbuffer.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = sphere.indexbuffer.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned int)*sphere.indexbuffer.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_IndexstadiumBuffer));
	});

	auto createstadium_topTask = (createlightPSTask && createVSTask && createHSTask && createDSTask).then([this]()
	{
		Mesh sphere = Mesh("Assets/sphere.obj");

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = sphere.uniqueVertList.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionUVNormal)*sphere.uniqueVertList.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_Vertstadium_topBuffer));

		m_indexstadium_topCount = sphere.indexbuffer.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = sphere.indexbuffer.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned int)*sphere.indexbuffer.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_Indexstadium_topBuffer));
	});

	auto createSkyboxTask = (createlightPSTask && createVSTask && createHSTask && createDSTask).then([this]()
	{
		Mesh sphere = Mesh("Assets/SkyboxCube.obj");

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = sphere.uniqueVertList.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionUVNormal)*sphere.uniqueVertList.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_VertSkyboxBuffer));

		m_indexSkyboxCount = sphere.indexbuffer.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = sphere.indexbuffer.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned int)*sphere.indexbuffer.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_IndexSkyboxBuffer));
	
		CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Assets/OutputCube.dds", nullptr, &m_SkyboxTex);
	});

	auto createPyramidsTask = (createInstanceVSTask && createPyramidPSTask && createGSTask).then([this]()
	{
		Mesh pyramid = Mesh("Assets/pyramid.obj");

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = pyramid.uniqueVertList.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionUVNormal)*pyramid.uniqueVertList.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_VertPyramidBuffer));

		m_indexPyramidCount = pyramid.indexbuffer.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = pyramid.indexbuffer.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned int)*pyramid.indexbuffer.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_IndexPyramidBuffer));
	});
	// Once the cube is loaded, the object is ready to be rendered.
	(createPyramidsTask  && createGroundTask).then([this]()
	{
		m_loadingComplete = true;
	});
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources(void)
{
	m_loadingComplete = false;
	m_vertexShader.Reset();
	m_inputLayout.Reset();
	m_pixelShader.Reset();
	m_constantBuffer.Reset();
	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();
}