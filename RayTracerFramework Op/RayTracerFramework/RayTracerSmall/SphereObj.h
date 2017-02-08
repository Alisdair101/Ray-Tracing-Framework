#pragma once

#include <stdlib.h>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <vector>
#include <iostream>
#include <chrono>
#include <ctime>
#include <cassert>

// Include Libraries
#include "Structures.h"

class SphereObj
{
public:
	std::string sphereName;
	Vec3f center;                           // position of the sphere
	float radius, radius2;                  // sphere radius and radius^2
	Vec3f surfaceColor, emissionColor;      // surface color and emission (light)
	float transparency, reflection;         // surface transparency and reflectivity
	float rotationSpeed;
	float startAngle;

	bool rootSphere;
	float orbitMagnitude;

	std::string parentSphereName;
	SphereObj* parentSphere;
	std::vector<SphereObj*> childrenSpheres;

	SphereObj(
		const std::string &sn,
		const bool	&root,
		const Vec3f &c,
		const float &r,
		const Vec3f &sc,
		const float &refl,
		const float &transp,
		const Vec3f &ec,
		const float &rs);
	SphereObj();
	~SphereObj();

#pragma region Get and Set Functions

	// Get/Set Sphere Name
	std::string GetSphereName() { return sphereName; }
	void SetSphereName(std::string sphereNewName) { sphereName = sphereNewName; }

	// Get/Set Root Sphere
	bool GetRootSphere() { return rootSphere; }
	void SetRootSphere(bool newRootSphere) { rootSphere = newRootSphere; }

	// Get/Set Position
	Vec3f GetPosition() { return center; }
	void  SetPosition(Vec3f position) { center = position; }

	// Get/Set Radius
	float GetRadius() { return radius; }
	void SetRadius(float newRadius) { radius = newRadius; radius2 = newRadius * newRadius; }

	// Get/Set Rotation Speed
	float GetRotationSpeed() const { return rotationSpeed; }
	void  SetRotationSpeed(float rotSpeed) { rotationSpeed = rotSpeed; }

	// Get/Set Sphere children 
	std::vector<SphereObj*> GetChildrenSpheres() { return childrenSpheres; }
	void AddSphereChild(SphereObj* sphere) { childrenSpheres.push_back(sphere); }

	// Get/Set Parent Sphere Name
	std::string GetParentSphereName() { return parentSphereName; }
	void SetParentSphereName(std::string newParentSphereName) { parentSphereName = newParentSphereName; }

	// Get/Set Parent Sphere
	SphereObj* GetParentSphere() { return parentSphere; }
	void SetParentSphere(SphereObj* sphere);

	// Get/Set Transparency
	float GetTransparency() { return transparency; }
	void  SetTransparency(float transp) { transparency = transp; }

	// Get/Set Reflection
	float GetReflection() { return reflection; }
	void  SetReflection(float refl) { reflection = refl; }

	// Get/Set Surface Colour
	Vec3f GetSurfaceColour() { return surfaceColor; }
	void  SetSurfaceColour(Vec3f surfColour) { surfaceColor = surfColour; }

	// Get/Set EmissionColor
	Vec3f GetEmissionColour() { return emissionColor; }
	void  SetEmissionColour(Vec3f emColor) { emissionColor = emColor; }

#pragma endregion

	// Compute a ray-sphere intersection using the geometric solution
	bool intersect(const Vec3f &rayorig, const Vec3f &raydir, float &t0, float &t1);

	// Update Children
	void UpdateChildren(float r, Vec3f parentPosition);

private:
	// Return position to rotate sphere around point
	Vec3f RotatePointAroundPoint(Vec3f sphere1Pos, float radian);
};