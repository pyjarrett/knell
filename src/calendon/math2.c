#include "math2.h"

#include "cn.h"
#include <math.h>

static const float pi = 3.14159265f;

float cnPlanarAngle_Degrees(CnPlanarAngle a)
{
	return a.degrees;
}

float cnPlanarAngle_Radians(CnPlanarAngle a)
{
	return a.degrees * pi / 180.0f;
}

CnPlanarAngle cnPlanarAngle_MakeDegrees(float d)
{
	return (CnPlanarAngle) { d };
}

CnPlanarAngle cnPlanarAngle_MakeRadians(float r)
{
	return (CnPlanarAngle) { r * 180.0f / pi };
}

CnPlanarAngle cnPlanarAngle_Add(CnPlanarAngle left, CnPlanarAngle right)
{
	return (CnPlanarAngle) { left.degrees + right.degrees };
}

CnPlanarAngle cnPlanarAngle_Sub(CnPlanarAngle left, CnPlanarAngle right)
{
	return (CnPlanarAngle) { left.degrees - right.degrees };
}

CnFloat2 cnFloat2_Make(float x, float y)
{
	return (CnFloat2) { x, y };
}

CnFloat2 cnFloat2_FromPolar(float radius, CnPlanarAngle theta)
{
	return (CnFloat2) { radius * cosf(cnPlanarAngle_Radians(theta)),
		radius * sinf(cnPlanarAngle_Radians(theta)) };
}

CnFloat2 cnFloat2_Add(CnFloat2 left, CnFloat2 right)
{
	return (CnFloat2) { left.x + right.x, left.y + right.y };
}

CnFloat2 cnFloat2_Sub(CnFloat2 left, CnFloat2 right)
{
	return (CnFloat2) { left.x - right.x, left.y - right.y };
}

CnFloat2 cnFloat2_Multiply(CnFloat2 v, float s)
{
	return (CnFloat2) { v.x * s, v.y * s };
}

CnFloat2 cnFloat2_Divide(CnFloat2 v, float s)
{
	return (CnFloat2) { v.x / s, v.y / s };
}

float cnFloat2_Length(CnFloat2 v)
{
	return sqrtf(v.x * v.x + v.y * v.y);
}

float cnFloat2_LengthSquared(CnFloat2 v)
{
	return v.x * v.x + v.y * v.y;
}

CnFloat2 cnFloat2_Normalize(CnFloat2 v)
{
	const float length = cnFloat2_Length(v);
	CN_ASSERT(length > 0.0f, "Cannot normalize a vector of 0 length.");
	return cnFloat2_Divide(v, length);
}

CnFloat2 cnFloat2_Midpoint(CnFloat2 left, CnFloat2 right)
{
	return cnFloat2_Divide(cnFloat2_Add(left, right), 2.0f);
}

float cnFloat2_DistanceSquared(CnFloat2 left, CnFloat2 right)
{
	return cnFloat2_LengthSquared(cnFloat2_Make(left.x - right.x, left.y - right.y));
}

CnFloat2 cnFloat2_Lerp(CnFloat2 from, CnFloat2 to, float alpha)
{
	CN_ASSERT(0.0f <= alpha && alpha <= 1.0f, "Alpha %f is not in range [0,1]", alpha);
	return cnFloat2_Add(cnFloat2_Multiply(from, 1.0f - alpha), cnFloat2_Multiply(to, alpha));
}

CnPlanarAngle cnFloat2_DirectionBetween(CnFloat2 from, CnFloat2 to)
{
	return cnPlanarAngle_MakeRadians(atan2f(to.y - from.y, to.x - from.x));
}

CnTransform2 cnTransform2_MakeIdentity(void)
{
	return (CnTransform2) {{
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f }
	}};
}

CnTransform2 cnTransform2_MakeTranslateXY(float x, float y)
{
	CN_ASSERT_FINITE_F32(x);
	CN_ASSERT_FINITE_F32(y);

	return (CnTransform2) {{
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{    x,    y, 1.0f }
	}};
}

CnTransform2 cnTransform2_MakeUniformScale(float scale)
{
	CN_ASSERT_FINITE_F32(scale);
	return (CnTransform2) {{
		{ scale, 0.0f, 0.0f },
		{ 0.0f, scale, 0.0f },
		{ 0.0f, 0.0f, scale }
	}};
}

CnTransform2 cnTransform2_MakeRotation(CnPlanarAngle angle)
{
	const float theta = cnPlanarAngle_Radians(angle);
	CN_ASSERT_FINITE_F32(theta);

	return (CnTransform2) {{
		{  cosf(theta), sinf(theta), 0.0f },
		{ -sinf(theta), cosf(theta), 0.0f },
		{         0.0f,        0.0f, 1.0f }
	}};
}

CnTransform2 cnTransform2_Combine(CnTransform2 first, CnTransform2 second)
{
	CnTransform2 result;
	// Loop over all positions in `result`.
	// TODO: This should be done with SIMD.
	for (uint32_t i = 0; i < 3; ++i) {
		for (uint32_t j = 0; j < 3; ++j) {
			// Multiply to find result[i][j].
			// Use a row from `first`, and a column from `second`.
			result.m[i][j] = 0.0f;
			for (uint32_t k = 0; k < 3; ++k) {
				result.m[i][j] += (first.m[k][j] * second.m[i][k]);
			}
			CN_ASSERT_FINITE_F32(result.m[i][j]);
		}
	}
	return result;
}

CnFloat2 cnMath2_TransformPoint(CnFloat2 point, CnTransform2 transform)
{
	return cnFloat2_Make(
		point.x * transform.m[0][0] + point.y * transform.m[1][0] + 1.0f * transform.m[2][0],
		point.x * transform.m[0][1] + point.y * transform.m[1][1] + 1.0f * transform.m[2][1]
	);
}

CnFloat2 cnMath2_TransformVector(CnFloat2 point, CnTransform2 transform)
{
	return cnFloat2_Make(
		point.x * transform.m[0][0] + point.y * transform.m[1][0],
		point.x * transform.m[0][1] + point.y * transform.m[1][1]
	);
}
