module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

export module MathOperations;

export extern const glm::vec3 TRANSLATION_IDENTITY(0);
export extern const glm::vec3 SCALE_IDENTITY(1);
export extern const glm::quat ROTATION_IDENTITY(1, 0, 0, 0);

// Equivalent to, but much faster than
// worldMatrix = glm::translate(glm::mat4(1.f), position);
// worldMatrix = glm::translate(worldMatrix, pivot);
// worldMatrix *= glm::mat4_cast(rotation);
// worldMatrix = glm::scale(worldMatrix, scale);
// worldMatrix = glm::translate(worldMatrix, -pivot);
//#define TEST_MODE 1
export void fromRotationTranslationScaleOrigin(const glm::quat& q, const glm::vec3& v, const glm::vec3& s, glm::mat4& out, const glm::vec3& pivot) {
	// ho tom bambadil
	// tom bombadillo
	// Retera was here
	// (This code is copied from the holy bible)
	float x = q.x;
	float y = q.y;
	float z = q.z;
	float w = q.w;
	float x2 = x + x;
	float y2 = y + y;
	float z2 = z + z;
	float xx = x * x2;
	float xy = x * y2;
	float xz = x * z2;
	float yy = y * y2;
	float yz = y * z2;
	float zz = z * z2;
	float wx = w * x2;
	float wy = w * y2;
	float wz = w * z2;
	float sx = s.x;
	float sy = s.y;
	float sz = s.z;
#ifdef TEST_MODE
	out[0][0] = (1 - (yy + zz)) * sx;
	out[1][0] = (xy + wz) * sx;
	out[2][0] = (xz - wy) * sx;
	out[3][0] = 0;
	out[0][1] = (xy - wz) * sy;
	out[1][1] = (1 - (xx + zz)) * sy;
	out[2][1] = (yz + wx) * sy;
	out[3][1] = 0;
	out[0][2] = (xz + wy) * sz;
	out[1][2] = (yz - wx) * sz;
	out[2][2] = (1 - (xx + yy)) * sz;
	out[3][2] = 0;
	out[0][3] = v.x + pivot.x - (out[0][0] * pivot.x + out[0][1] * pivot.y + out[0][2] * pivot.z);
	out[1][3] = v.y + pivot.y - (out[1][0] * pivot.x + out[1][1] * pivot.y + out[1][2] * pivot.z);
	out[2][3] = v.z + pivot.z - (out[2][0] * pivot.x + out[2][1] * pivot.y + out[2][2] * pivot.z);
	out[3][3] = 1;
#else
	out[0][0] = (1 - (yy + zz)) * sx;
	out[0][1] = (xy + wz) * sx;
	out[0][2] = (xz - wy) * sx;
	out[0][3] = 0;
	out[1][0] = (xy - wz) * sy;
	out[1][1] = (1 - (xx + zz)) * sy;
	out[1][2] = (yz + wx) * sy;
	out[1][3] = 0;
	out[2][0] = (xz + wy) * sz;
	out[2][1] = (yz - wx) * sz;
	out[2][2] = (1 - (xx + yy)) * sz;
	out[2][3] = 0;
	out[3][0] = v.x + pivot.x - (out[0][0] * pivot.x + out[1][0] * pivot.y + out[2][0] * pivot.z);
	out[3][1] = v.y + pivot.y - (out[0][1] * pivot.x + out[1][1] * pivot.y + out[2][1] * pivot.z);
	out[3][2] = v.z + pivot.z - (out[0][2] * pivot.x + out[1][2] * pivot.y + out[2][2] * pivot.z);
	out[3][3] = 1;
#endif
}

glm::quat ghostwolfSquad(const glm::quat a, const glm::quat aOutTan, const glm::quat bInTan, const glm::quat b, float t) {
	glm::quat temp1;
	glm::quat temp2;
	temp1 = glm::slerp(a, b, t);
	temp2 = glm::slerp(aOutTan, bInTan, t);
	return glm::slerp(temp1, temp2, 2 * t * (1 - t));
}

float hermite(float a, float aOutTan, float bInTan, float b, float t) {
	float factorTimes2 = t * t;
	float factor1 = factorTimes2 * (2 * t - 3) + 1;
	float factor2 = factorTimes2 * (t - 2) + t;
	float factor3 = factorTimes2 * (t - 1);
	float factor4 = factorTimes2 * (3 - 2 * t);
	return (a * factor1) + (aOutTan * factor2) + (bInTan * factor3) + (b * factor4);
}
float bezier(float a, float aOutTan, float bInTan, float b, float t) {
	float invt = 1 - t;
	float factorSquared = t * t;
	float inverseFactorSquared = invt * invt;
	float factor1 = inverseFactorSquared * invt;
	float factor2 = 3 * t * inverseFactorSquared;
	float factor3 = 3 * factorSquared * invt;
	float factor4 = factorSquared * t;
	return (a * factor1) + (aOutTan * factor2) + (bInTan * factor3) + (b * factor4);
}

// template <typename T>
// inline void interpolate(T& out, const T* start, const T* outTan, const T* inTan, const T* end, float t, int interpolationType) {
//	out = *start;
// }

export float interpolate(const float start, const float outTan, const float inTan, const float end, float t, int interpolationType) {
	switch (interpolationType) {
		case 1: // LINEAR
			return glm::mix(start, end, t);
		case 2: // HERMITE
			return hermite(start, outTan, inTan, end, t);
		case 3: // BEZIER
			return bezier(start, outTan, inTan, end, t);
		default:
			return start;
	}
}

export glm::vec3 interpolate(const glm::vec3 start, const glm::vec3 outTan, const glm::vec3 inTan, const glm::vec3 end, float t, int interpolationType) {
	switch (interpolationType) {
		case 1: // LINEAR
			return glm::mix(start, end, t);
		case 2: { // HERMITE
			glm::vec3 out;
			out.x = hermite(start.x, outTan.x, inTan.x, end.x, t);
			out.y = hermite(start.y, outTan.y, inTan.y, end.y, t);
			out.z = hermite(start.z, outTan.z, inTan.z, end.z, t);
			return out;
		}
		case 3: { // BEZIER
			glm::vec3 out;
			out.x = bezier(start.x, outTan.x, inTan.x, end.x, t);
			out.y = bezier(start.y, outTan.y, inTan.y, end.y, t);
			out.z = bezier(start.z, outTan.z, inTan.z, end.z, t);
			return out;
		}
		default:
			return start;
	}
}

export glm::quat interpolate(const glm::quat start, const glm::quat outTan, const glm::quat inTan, const glm::quat end, float t, int interpolationType) {
	switch (interpolationType) {
		case 1: // LINEAR
			return glm::slerp(start, end, t);
		case 2: // HERMITE
			// GLM uses both {x, y, z, w} and {w, x, y, z} convention, in different places, sometimes.
			// Their squad is {w, x, y, z} but we are elsewhere using {x, y, z, w}, so we will
			// continue using the copy of the Matrix Eater "ghostwolfSquad" for now.
			// out = glm::squad(*start, *outTan, *inTan, *end, t);
			return ghostwolfSquad(start, outTan, inTan, end, t);
		case 3: // BEZIER
			// GLM uses both {x, y, z, w} and {w, x, y, z} convention, in different places, sometimes.
			// Their squad is {w, x, y, z} but we are elsewhere using {x, y, z, w}, so we will
			// continue using the copy of the Matrix Eater "ghostwolfSquad" for now.
			// out = glm::squad(*start, *outTan, *inTan, *end, t);
			return ghostwolfSquad(start, outTan, inTan, end, t);
		default:
			return start;
	}
}
export uint32_t interpolate(const uint32_t start, const uint32_t, const uint32_t, const uint32_t, float, int) {
	return start;
}

export glm::quat safeQuatLookAt(
	glm::vec3 const& lookFrom,
	glm::vec3 const& lookTo,
	glm::vec3 const& up,
	glm::vec3 const& alternativeUp) {
	glm::vec3 direction = lookTo - lookFrom;
	float directionLength = glm::length(direction);

	// Check if the direction is valid; Also deals with NaN
	if (!(directionLength > 0.0001))
		return glm::quat(1, 0, 0, 0); // Just return identity

	// Normalize direction
	direction /= directionLength;

	// Is the normal up (nearly) parallel to direction?
	if (glm::abs(glm::dot(direction, up)) > .9999f) {
		// Use alternative up
		return glm::quatLookAt(direction, alternativeUp);
	} else {
		return glm::quatLookAt(direction, up);
	}
}