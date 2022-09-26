#include "Camera.h"

SphericalCoord::SphericalCoord(const CartesianCoord& cartesian) {
	radius = std::sqrtf(cartesian.x * cartesian.x + cartesian.y * cartesian.y + cartesian.z * cartesian.z);
	theta = std::acosf(cartesian.y / radius);
	phi = std::acosf(cartesian.x / std::sqrtf(cartesian.x * cartesian.x + cartesian.z * cartesian.z));
}

void SphericalCoord::fromCartesian(const CartesianCoord& cartesian) {
	radius = std::sqrtf(cartesian.x * cartesian.x + cartesian.y * cartesian.y + cartesian.z * cartesian.z);
	theta = std::acosf(cartesian.y / radius);
	phi = std::acosf(cartesian.x / std::sqrtf(cartesian.x * cartesian.x + cartesian.z * cartesian.z));
}