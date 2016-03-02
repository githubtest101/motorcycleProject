#include "SceneNode.h"

SceneNode::SceneNode()
{
	m_p_model = NULL;
	m_x = 0.0f; m_y = 0.0f; m_z = 0.0f;
	m_xangle = 0.0f; m_yangle = 0.0f; m_zangle = 0.0f;
	m_scale = 1.0f;
}

void SceneNode::addChildNode(SceneNode *n)
{
	m_children.push_back(n);
}

bool SceneNode::detatchNode(SceneNode *n)
{
	// traverse tree to find node to detatch
	for (int i = 0; i < m_children.size(); i++)
	{
		if (n == m_children[i])
		{
			m_children.erase(m_children.begin() + i);
			return true;
		}
		if (m_children[i]->detatchNode(n) == true) return true;
	}
	return false; // node not in this tree
}

void SceneNode::execute(XMMATRIX *world, XMMATRIX* view, XMMATRIX* projection)
{
	//local_world matrix, used to calc local transforms for this node
	XMMATRIX local_world = XMMatrixIdentity();

	local_world = XMMatrixRotationX(XMConvertToRadians(m_xangle));
	local_world *= XMMatrixRotationY(XMConvertToRadians(m_yangle));
	local_world *= XMMatrixRotationZ(XMConvertToRadians(m_zangle));
	local_world *= XMMatrixScaling(m_scale, m_scale, m_scale);
	local_world *= XMMatrixTranslation(m_x, m_y, m_z);

	//passed in world matrix contains concatenated transformations of all 
	//parent nodes so that this nodes transformations are relative to those
	local_world *= *world;

	// only draw if there is a model attached
	if (m_p_model) m_p_model->Draw(&local_world, view, projection);

	// traverse all child nodes, passing in the concatenated world matrix
	for (int i = 0; i< m_children.size(); i++)
	{
		m_children[i]->execute(&local_world, view, projection);
	}
}

void  SceneNode::UpdateCollisionTree(XMMATRIX* world, float scale)
{
	// LocalWorld used to calculate local transforms for each node
	XMMATRIX LocalWorld = XMMatrixIdentity();

	LocalWorld = XMMatrixRotationX(XMConvertToRadians(m_xangle));
	LocalWorld *= XMMatrixRotationY(XMConvertToRadians(m_yangle));
	LocalWorld *= XMMatrixRotationZ(XMConvertToRadians(m_zangle));
	LocalWorld *= XMMatrixScaling(m_scale, m_scale, m_scale);
	LocalWorld *= XMMatrixTranslation(m_x, m_y, m_z);

	//LocalWorld is multiplied by the passed in world matrix that contains the concatenated...
	//...transformations of all parent nodes so that this nodes transformations are relative to those
	LocalWorld *= *world;

	// calc the world space scale of this object, is needed to calculate the  
	// correct bounding sphere radius of an object in a scaled hierarchy
	m_world_scale = scale * m_scale;

	XMVECTOR v;
	if (m_p_model)  // this wont work ******************************
	{
		v = XMVectorSet(m_p_model->GetBoundingSphere_x(), m_p_model->GetBoundingSphere_y(),
			m_p_model->GetBoundingSphere_z(), 0.0);
	}
	else
	{
		v = XMVectorSet(0, 0, 0, 0); // no model, default to 0
	}

	// find and store world space bounding sphere centre
	v = XMVector3Transform(v, LocalWorld);
	m_world_centre_x = XMVectorGetX(v);
	m_world_centre_y = XMVectorGetY(v);
	m_world_centre_z = XMVectorGetZ(v);

	// Update all child nodes, passing in the final world matrix and scale
	for (int i = 0; i< m_children.size(); i++)
	{
		m_children[i]->UpdateCollisionTree(&LocalWorld, m_world_scale);
	}
}

bool SceneNode::CheckCollision(SceneNode* compare_tree)
{
	return CheckCollision(compare_tree, this);
}

bool SceneNode::CheckCollision(SceneNode* compare_tree, SceneNode* object_tree_root)
{
	// check to see if root of tree being compared is same as root node of object tree being checked
	// i.e. stop object node and children being checked against each other
	if (object_tree_root == compare_tree) return false;

	// only check for collisions if both nodes contain a model
	if (m_p_model && compare_tree->m_p_model)
	{
		XMVECTOR v1 = GetWorldCentrePosition();
		XMVECTOR v2 = compare_tree->GetWorldCentrePosition();
		XMVECTOR vdiff = v1 - v2;

		//XMVECTOR a = XMVector3Length(vdiff);
		float x1 = XMVectorGetX(v1);
		float x2 = XMVectorGetX(v2);
		float y1 = XMVectorGetY(v1);
		float y2 = XMVectorGetY(v2);
		float z1 = XMVectorGetZ(v1);
		float z2 = XMVectorGetZ(v2);
		
		float dx = x1 - x2;
		float dy = y1 - y2;
		float dz = z1 - z2;

		/* this is a more optimised method
		float dx = pow(x1 - x2, 2);
		float dy = pow(y1 - y2, 2);
		float dz = pow(z1 - z2, 2);
		float distance_squared = (dx + dy + dz);
		*/
		// check bounding sphere collision
		if ((sqrt(dx*dx + dy*dy + dz*dz)) < (compare_tree->m_p_model->GetBoundingSphereRadius() * compare_tree->m_world_scale) +
			(this->m_p_model->GetBoundingSphereRadius() * m_world_scale))
		{
			return true;
		}
	}

	// iterate through compared tree child nodes
	for (int i = 0; i< compare_tree->m_children.size(); i++)
	{
		// check for collsion against all compared tree child nodes 
		if (CheckCollision(compare_tree->m_children[i], object_tree_root) == true) return true;
	}

	// iterate through composite object child nodes
	for (int i = 0; i< m_children.size(); i++)
	{
		// check all the child nodes of the composite object against compared tree
		if (m_children[i]->CheckCollision(compare_tree, object_tree_root) == true) return true;
	}

	return false;

}

void SceneNode::SetModel(Model* model)
{	
	m_p_model = model;
}

XMVECTOR SceneNode::GetWorldCentrePosition()
{
	return XMVectorSet(m_world_centre_x, m_world_centre_y, m_world_centre_z, 0.0);
}

void SceneNode::setNodePosition(float x, float y, float z){	m_x = x;m_y = y;m_z = z;}
void SceneNode::setXNodeAngle(float num){m_xangle += num;}
void SceneNode::setYNodeAngle(float num){m_yangle += num;}
void SceneNode::setZNodeAngle(float num){m_zangle += num;}
void SceneNode::setNodeScale(float num){m_scale = num;}

float SceneNode::getXNodePosition(){ return m_x; }
float SceneNode::getYNodePosition(){ return m_y; }
float SceneNode::getZNodePosition(){ return m_z; }

bool SceneNode::AlterX(float in, SceneNode* rootnode)
{
	float old_x = m_x;	// save current state 
	m_x += in;		// update state

	XMMATRIX identity = XMMatrixIdentity();

	// since state has changed, need to update collision tree
	// this basic system requires entire hirearchy to be updated
	// so start at root node passing in identity matrix
	rootnode->UpdateCollisionTree(&identity, 1.0);

	// check for collision of this node (and children) against all other nodes
	if (CheckCollision(rootnode) == true)
	{
		// if collision restore state
		m_x = old_x;
		OutputDebugString("AlterX Collision ");
		return true;
	}
	return false;
}

bool SceneNode::Shoot(float DestroyValue, SceneNode* rootnode)
{
	m_z = m_z++;

	XMMATRIX identity = XMMatrixIdentity();
	rootnode->UpdateCollisionTree(&identity, 1.0);
	if (CheckCollision(rootnode) == true)
	{
		m_z = DestroyValue;
		OutputDebugString("Hit ");
		return true;
	}
	return false;
}


bool SceneNode::LookAt_XZ(float x, float z, SceneNode* rootnode)
{
	//i think this is fucked
	float old_x = m_x;
	float old_z = m_z;
	float m_dx = x - m_x;
	float m_dz = z - m_z;
	float m_y_angle = atan2(m_dx, m_dz) * (180.0 / XM_PI);

	XMMATRIX identity = XMMatrixIdentity();
	rootnode->UpdateCollisionTree(&identity, 1.0);
	if (CheckCollision(rootnode) == true)
	{
		m_x = old_x;
		m_z = old_z;
		OutputDebugString("Lookat Collision ");
		return true;
	}
	return false;
}