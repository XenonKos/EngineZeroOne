#pragma once
#include <cmath>

#include "D3D12App.h"

struct CartesianCoord;

struct SphericalCoord {
	SphericalCoord(float theta, float phi, float radius)
		: theta(theta), phi(phi), radius(radius) {}
	SphericalCoord(const CartesianCoord& cartesian);

	void fromCartesian(const CartesianCoord& cartesian);

	float theta;
	float phi;
	float radius;
};

struct CartesianCoord {
	CartesianCoord(float x, float y, float z)
		: x(x), y(y), z(z) {}

	CartesianCoord(const SphericalCoord& spherical) {
		y = spherical.radius * std::cosf(spherical.theta);
		x = spherical.radius * std::sinf(spherical.theta) * std::cosf(spherical.phi);
		z = spherical.radius * std::sinf(spherical.theta) * std::sinf(spherical.phi);
	}

	void fromSpherical(const SphericalCoord& spherical) {
		y = spherical.radius * std::cosf(spherical.theta);
		x = spherical.radius * std::sinf(spherical.theta) * std::cosf(spherical.phi);
		z = spherical.radius * std::sinf(spherical.theta) * std::sinf(spherical.phi);
	}

	float x;
	float y;
	float z;
};


struct Camera {
	Camera() = default;

	Camera(XMFLOAT3 position,
		XMFLOAT3 focusDirection,
		XMFLOAT3 upDirection = XMFLOAT3(0.0f, 1.0f, 0.0f),
		float fov = XM_PIDIV4, float nearZ = 0.1f, float farZ = 1000.0f)
		: mPosition(position),
		mFov(fov),
		mNearZ(nearZ),
		mFarZ(farZ) {

		// Focus Direction
		XMVECTOR focus = XMVector3Normalize(XMLoadFloat3(&focusDirection));
		XMStoreFloat3(&mFocusDirection, focus);

		// Up Direction
		XMVECTOR up = XMLoadFloat3(&upDirection);
		up = XMVector3Normalize(up - XMVectorMultiply(XMVector3Dot(up, focus), focus));
		XMStoreFloat3(&mUpDirection, up);

		// Right Direction
		XMStoreFloat3(&mRightDirection,
			XMVector3Normalize(XMVector3Cross(up, focus))
		);

		// View Matrix
		UpdateViewMatrix();
	}

	~Camera() {}

	void SetLens(UINT width, UINT height) {
		mWidth = width;
		mHeight = height;

		UpdateProjectionMatrix();
	}

	// 俯仰
	void Pitch(float dTheta) {
		// 旋转矩阵
		XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mRightDirection), dTheta);

		XMStoreFloat3(&mUpDirection, XMVector3TransformNormal(XMLoadFloat3(&mUpDirection), R));
		XMStoreFloat3(&mFocusDirection, XMVector3TransformNormal(XMLoadFloat3(&mFocusDirection), R));
	}
	
	// 偏航
	void Yaw(float dPhi) {
		//XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mUpDirection), dPhi);

		//XMStoreFloat3(&mRightDirection, XMVector3TransformNormal(XMLoadFloat3(&mRightDirection), R));
		//XMStoreFloat3(&mFocusDirection, XMVector3TransformNormal(XMLoadFloat3(&mFocusDirection), R));

		XMMATRIX R = XMMatrixRotationY(dPhi);
		XMStoreFloat3(&mUpDirection, XMVector3TransformNormal(XMLoadFloat3(&mUpDirection), R));
		XMStoreFloat3(&mRightDirection, XMVector3TransformNormal(XMLoadFloat3(&mRightDirection), R));
		XMStoreFloat3(&mFocusDirection, XMVector3TransformNormal(XMLoadFloat3(&mFocusDirection), R));
	}

	// 六向移动
	void MoveForward() {
		XMVECTOR dis = XMVectorReplicate(mMoveStride);
		XMVECTOR focus = XMLoadFloat3(&mFocusDirection);
		XMVECTOR pos = XMLoadFloat3(&mPosition);

		XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(dis, focus, pos));
	}

	void MoveBackward(){
		XMVECTOR dis = XMVectorReplicate(-mMoveStride);
		XMVECTOR focus = XMLoadFloat3(&mFocusDirection);
		XMVECTOR pos = XMLoadFloat3(&mPosition);

		XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(dis, focus, pos));
	}

	void MoveRight() {
		XMVECTOR dis = XMVectorReplicate(mMoveStride);
		XMVECTOR right = XMLoadFloat3(&mRightDirection);
		XMVECTOR pos = XMLoadFloat3(&mPosition);

		XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(dis, right, pos));
	}

	void MoveLeft() {
		XMVECTOR dis = XMVectorReplicate(-mMoveStride);
		XMVECTOR right = XMLoadFloat3(&mRightDirection);
		XMVECTOR pos = XMLoadFloat3(&mPosition);

		XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(dis, right, pos));
	}

	// 上下移动以绝对坐标为准
	void MoveUp() {
		XMVECTOR dis = XMVectorReplicate(mMoveStride);
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		XMVECTOR pos = XMLoadFloat3(&mPosition);

		XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(dis, up, pos));
	}

	void MoveDown() {
		XMVECTOR dis = XMVectorReplicate(-mMoveStride);
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		XMVECTOR pos = XMLoadFloat3(&mPosition);

		XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(dis, up, pos));
	}

	void UpdateViewMatrix() {
		XMVECTOR pos = XMLoadFloat3(&mPosition);
		XMVECTOR focus = XMLoadFloat3(&mFocusDirection);
		XMVECTOR up = XMLoadFloat3(&mUpDirection);

		XMMATRIX view = XMMatrixLookToLH(pos, focus, up);
		XMStoreFloat4x4(&mViewMatrix, view);
	}

	void UpdateProjectionMatrix() {
		XMMATRIX proj = XMMatrixPerspectiveFovLH(mFov, static_cast<float>(mWidth) / mHeight, mNearZ, mFarZ);
		XMStoreFloat4x4(&mProjectionMatrix, proj);
	}

	XMMATRIX ViewMatrix() const {
		return XMLoadFloat4x4(&mViewMatrix);
	}

	XMMATRIX ProjectionMatrix() const {
		return XMLoadFloat4x4(&mProjectionMatrix);
	}

	XMFLOAT3 SphericalPos() const {
		SphericalCoord sphericalPos(CartesianCoord(mPosition.x, mPosition.y, mPosition.z));
		return XMFLOAT3(sphericalPos.theta, sphericalPos.phi, sphericalPos.radius);
	}

	XMFLOAT3 CartesianPos() const {
		return mPosition;
	}

	float Clamp(float value, float lowerbound, float upperbound) {
		if (value < lowerbound) {
			return lowerbound;
		}
		else if (value > upperbound) {
			return upperbound;
		}

		return value;
	}

	//SphericalCoord mPosition;
	XMFLOAT3 mPosition;
	XMFLOAT3 mFocusDirection;
	XMFLOAT3 mUpDirection;
	XMFLOAT3 mRightDirection;

	XMFLOAT4X4 mViewMatrix;
	XMFLOAT4X4 mProjectionMatrix;

	// Camera移动设置
	float mMoveStride = 0.1f;

	// Frustum信息
	UINT mWidth;
	UINT mHeight;
	float mFov;
	float mNearZ;
	float mFarZ;
};