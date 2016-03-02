#pragma once

#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <dxerr.h>
#include <dinput.h>
#define _XM_NO_INTRINSICS_
#define XM_NO_ALIGNMENT
#include <xnamath.h>
#include <windows.h>
#include "Model.h"

class SceneNode
{
private:
	Model* m_p_model;
	vector<SceneNode*> m_children;
	float m_x, m_y, m_z;
	float m_xangle, m_yangle, m_zangle;
	float m_scale;
	float m_world_centre_x, m_world_centre_y, m_world_centre_z;
	float m_world_scale;


public:
	SceneNode();
	void addChildNode(SceneNode *n);
	bool detatchNode(SceneNode *n);
	void SetModel(Model* m_p_model);
	void execute(XMMATRIX *world, XMMATRIX* view, XMMATRIX* projection);
	void UpdateCollisionTree(XMMATRIX* world, float scale);
	bool CheckCollision(SceneNode* compare_tree);
	bool CheckCollision(SceneNode* compare_tree, SceneNode* object_tree_root);

	XMVECTOR GetWorldCentrePosition();

	void setNodePosition(float x, float y, float z);
	void setXNodeAngle(float num);
	void setYNodeAngle(float num);
	void setZNodeAngle(float num);
	void setNodeScale(float num);
	float getXNodePosition();
	float getYNodePosition();
	float getZNodePosition();

	bool AlterX(float in, SceneNode* rootnode);
	bool Shoot(float DestroyValue, SceneNode* rootnode);
	bool MoveForward(float distance, SceneNode* rootnode); //using IncX as example
	bool LookAt_XZ(float x, float z, SceneNode* rootnode); //using IncX as example
};