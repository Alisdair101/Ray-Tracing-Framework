#include "SphereObj.h"

SphereObj::SphereObj(
	const std::string &sn,
	const bool	&root,
	const Vec3f &c,
	const float &r,
	const Vec3f &sc,
	const float &refl = 0,
	const float &transp = 0,
	const Vec3f &ec = 0,
	const float &rs = 0) : 
	sphereName(sn), rootSphere(root), center(c), radius(r), radius2(r * r), surfaceColor(sc), reflection(refl), transparency(transp), emissionColor(ec), rotationSpeed(rs)
{
	parentSphereName = "";
	parentSphere = nullptr;
}

SphereObj::SphereObj()
{
	parentSphere = nullptr;
}

SphereObj::~SphereObj()
{
}

bool SphereObj::intersect(const Vec3f &rayorig, const Vec3f &raydir, float &t0, float &t1)
{
	Vec3f l = center - rayorig;
	float tca = l.dot(raydir);
	if (tca < 0) return false;
	float d2 = l.dot(l) - tca * tca;
	if (d2 > radius2) return false;
	float thc = sqrt(radius2 - d2);
	t0 = tca - thc;
	t1 = tca + thc;

	return true;
}

// Update Children
void SphereObj::UpdateChildren(float r, Vec3f parentPosition)
{
	if (parentSphere != nullptr)
	{
		center = RotatePointAroundPoint(parentPosition, rotationSpeed * (r+startAngle));
	}

	for each(SphereObj* sphere in childrenSpheres)
	{
		sphere->UpdateChildren(r, center);
	}
}

// Return position to rotate sphere around point
Vec3f SphereObj::RotatePointAroundPoint(Vec3f sphere1Pos, float radian)
{
	Vec3f newPos;
	newPos.x = sphere1Pos.x + (orbitMagnitude * cosf(radian));
	newPos.y = sphere1Pos.y + (orbitMagnitude * sinf(radian));
	newPos.z = -10.0f;

	return newPos;
}

void SphereObj::SetParentSphere(SphereObj* sphere)
{
	parentSphere = sphere;
	Vec3f distance = parentSphere->GetPosition() - center;
	orbitMagnitude = sqrt((distance.x * distance.x) + (distance.y * distance.y));

	float dot = (100.0f*distance.x) + (0.0f*distance.y);
	float det = (100.0f*distance.y) + (0.0f*distance.x);
	startAngle = atan2(det, -dot) * 5;
}