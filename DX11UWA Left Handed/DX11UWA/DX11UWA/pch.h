#pragma once

#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>
#include <concrt.h>
#include <vector>
#include "Content\ShaderStructures.h"
#include "Common\DDSTextureLoader.h"

using namespace DX11UWA;
using namespace std;
using namespace DirectX;
using namespace Windows::Foundation;

class Mesh
{
public:
	Mesh() {};
	Mesh(const char* filename);
	~Mesh();

	vector<VertexPositionUVNormal> uniqueVertList;
	vector<unsigned int> indexbuffer;
private:


};

class SceneObject
{
public:
	SceneObject();
	~SceneObject();

private:

};

SceneObject::SceneObject()
{
}

SceneObject::~SceneObject()
{
}


