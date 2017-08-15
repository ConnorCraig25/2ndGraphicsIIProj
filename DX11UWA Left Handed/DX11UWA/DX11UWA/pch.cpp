#include "pch.h"

Mesh::Mesh(const char* filename)
{
	vector<XMFLOAT3> pos;
	vector<XMFLOAT3> uvs;
	vector<XMFLOAT3> normals;
	vector<XMINT3> trindices;
	FILE * file = nullptr;
	fopen_s(&file, filename, "r");
	if (file != NULL)
	{
		while (true)
		{
			char lineHeader[128];
			// read the first word of the line
			int res = fscanf_s(file, "%s", lineHeader, 128);
			if (res == EOF)
				break; // EOF = End Of File. Quit the loop.

			if (strcmp(lineHeader, "v") == 0)
			{
				XMFLOAT3 postmp;
				fscanf_s(file, "%f %f %f\n", &postmp.x, &postmp.y, &postmp.z);
				pos.push_back(postmp);
			}
			else if (strcmp(lineHeader, "vt") == 0)
			{
				XMFLOAT3 uvtmp;
				fscanf_s(file, "%f %f\n", &uvtmp.x, &uvtmp.y);
				if (uvtmp.x > 1.0)
				{
					uvtmp.x /=    520.0f;
						uvtmp.y /= 520.0f;
				}
				uvs.push_back(uvtmp);
			}
			else if (strcmp(lineHeader, "vn") == 0) {
				XMFLOAT3 normaltmp;
				fscanf_s(file, "%f %f %f\n", &normaltmp.x, &normaltmp.y, &normaltmp.z);
				normals.push_back(normaltmp);
			}
			else if (strcmp(lineHeader, "f") == 0)
			{
				XMINT3 index1;
				XMINT3 index2;
				XMINT3 index3;
				fscanf_s(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &index1.x, &index1.y, &index1.z,
					&index2.x, &index2.y, &index2.z,
					&index3.x, &index3.y, &index3.z);
				trindices.push_back(index1);
				trindices.push_back(index2);
				trindices.push_back(index3);
			}
		}

	}
	for (unsigned int i = 0; i < trindices.size(); i++)
	{

		VertexPositionUVNormal tempVert = VertexPositionUVNormal();

		tempVert.pos = pos[trindices[i].x - 1];
		tempVert.uv = uvs[trindices[i].y - 1];
		tempVert.normal = normals[trindices[i].z - 1];

		uniqueVertList.push_back(tempVert);
		indexbuffer.push_back(i);
		//if (uniqueVertList.size() == 0)
		//{
		//	uniqueVertList.push_back(tempVert);
		//	indexbuffer.push_back(uniqueVertList.size() - 1);
		//}
		//else
		//for (unsigned int i = 0; i < uniqueVertList.size(); i++)
		//{
		//	 if (tempVert.normal.x == uniqueVertList[i].normal.x
		//			&& tempVert.normal.y == uniqueVertList[i].normal.y
		//			&& tempVert.normal.z == uniqueVertList[i].normal.z
		//			&& tempVert.pos.x == uniqueVertList[i].pos.x
		//			&& tempVert.pos.y == uniqueVertList[i].pos.y
		//			&& tempVert.pos.z == uniqueVertList[i].pos.z
		//			&& tempVert.uv.x == uniqueVertList[i].uv.x
		//			&& tempVert.uv.y == uniqueVertList[i].uv.y
		//			&& tempVert.uv.z == uniqueVertList[i].uv.z)
		//	{
		//		indexbuffer.push_back(i);
		//		break;
		//	}
		//	else
		//	{
		//		uniqueVertList.push_back(tempVert);
		//		indexbuffer.push_back(uniqueVertList.size() - 1);
		//	}
		//}
	}
}
Mesh::~Mesh()
{
	uniqueVertList.clear();
	indexbuffer.clear();
}
