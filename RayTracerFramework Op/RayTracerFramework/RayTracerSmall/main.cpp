// [header]
// A very basic raytracer example.
// [/header]
// [compile]
// c++ -o raytracer -O3 -Wall raytracer.cpp
// [/compile]
// [ignore]
// Copyright (C) 2012  www.scratchapixel.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// [/ignore]
#include <stdlib.h>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <vector>
#include <iostream>
#include <chrono>
#include <ctime>
#include <cassert>

// Windows only
#include <algorithm>
#include <sstream>
#include <string.h>

// Include Libraries
#include "windows.h"
#include "tinyxml2.h"
#include <thread>

// Include Classes
#include "SphereObj.h"
#include "Structures.h"
#include "ThreadManager.h"

#if defined __linux__ || defined __APPLE__
// "Compiled for Linux
#else
// Windows doesn't define these values by default, Linux does
#define M_PI 3.141592653589793
#define INFINITY 1e8
#endif

//[comment]
// This variable controls the maximum recursion depth
//[/comment]
#define MAX_RAY_DEPTH 5

// Global Variables
ThreadManager* threadManager;
std::ofstream frameLogFile;

float mix(const float &a, const float &b, const float &mix)
{
	return b * mix + a * (1 - mix);
}

//[comment]
// This is the main trace function. It takes a ray as argument (defined by its origin
// and direction). We test if this ray intersects any of the geometry in the scene.
// If the ray intersects an object, we compute the intersection point, the normal
// at the intersection point, and shade this point using this information.
// Shading depends on the surface property (is it transparent, reflective, diffuse).
// The function returns a color for the ray. If the ray intersects an object that
// is the color of the object at the intersection point, otherwise it returns
// the background color.
//[/comment]
Vec3f trace(const Vec3f &rayorig, const Vec3f &raydir, const std::vector<SphereObj*> spheres, const int &depth)
{
	//if (raydir.length() != 1) std::cerr << "Error " << raydir << std::endl;
	float tnear = INFINITY;
	const SphereObj* sphere = NULL;

	// find intersection of this ray with the sphere in the scene
	for (unsigned i = 0; i < spheres.size(); ++i)
	{
		float t0 = INFINITY, t1 = INFINITY;
		if (spheres[i]->intersect(rayorig, raydir, t0, t1))
		{
			if (t0 < 0)
			{
				t0 = t1;
			}

			if (t0 < tnear)
			{
				tnear = t0;
				sphere = spheres[i];
			}
		}
	}

	// if there's no intersection return black or background color
	if (!sphere)
	{
		return Vec3f(2);
	}

	Vec3f surfaceColor = 0; // color of the ray/surfaceof the object intersected by the ray
	Vec3f phit = rayorig + raydir * tnear; // point of intersection
	Vec3f nhit = phit - sphere->center; // normal at the intersection point
	nhit.normalize(); // normalize normal direction
					  // If the normal and the view direction are not opposite to each other
					  // reverse the normal direction. That also means we are inside the sphere so set
					  // the inside bool to true. Finally reverse the sign of IdotN which we want
					  // positive.
	float bias = 1e-4; // add some bias to the point from which we will be tracing
	bool inside = false;

	if (raydir.dot(nhit) > 0)
	{
		nhit = -nhit;
		inside = true;
	}

	if ((sphere->transparency > 0 || sphere->reflection > 0) && depth < MAX_RAY_DEPTH) 
	{
		float facingratio = -raydir.dot(nhit);

		// change the mix value to tweak the effect
		float fresneleffect = mix(pow(1 - facingratio, 3), 1, 0.1);

		// compute reflection direction (not need to normalize because all vectors are already normalized)
		Vec3f refldir = raydir - nhit * 2 * raydir.dot(nhit);
		refldir.normalize();
		Vec3f reflection = trace(phit + nhit * bias, refldir, spheres, depth + 1);
		Vec3f refraction = 0;

		// if the sphere is also transparent compute refraction ray (transmission)
		if (sphere->transparency)
		{
			float ior = 1.1, eta = (inside) ? ior : 1 / ior; // are we inside or outside the surface?
			float cosi = -nhit.dot(raydir);
			float k = 1 - eta * eta * (1 - cosi * cosi);
			Vec3f refrdir = raydir * eta + nhit * (eta *  cosi - sqrt(k));
			refrdir.normalize();
			refraction = trace(phit - nhit * bias, refrdir, spheres, depth + 1);
		}
		// the result is a mix of reflection and refraction (if the sphere is transparent)
		surfaceColor = (reflection * fresneleffect + refraction * (1 - fresneleffect) * sphere->transparency) * sphere->surfaceColor;
	}
	else 
	{
		// it's a diffuse object, no need to raytrace any further
		for (unsigned i = 0; i < spheres.size(); ++i)
		{
			if (spheres[i]->emissionColor.x > 0)
			{
				// this is a light
				Vec3f transmission = 1;
				Vec3f lightDirection = spheres[i]->center - phit;
				lightDirection.normalize();
				for (unsigned j = 0; j < spheres.size(); ++j)
				{
					if (i != j)
					{
						float t0, t1;

						if (spheres[j]->intersect(phit + nhit * bias, lightDirection, t0, t1))
						{
							transmission = 0;
							break;
						}
					}
				}

				surfaceColor += sphere->surfaceColor * transmission * max(float(0), nhit.dot(lightDirection)) * spheres[i]->emissionColor;
			}
		}
	}

	return surfaceColor + sphere->emissionColor;
}

void renderPixel(Vec3f* pixel, unsigned x, unsigned y, float invWidth, float invHeight, float angle, float aspectratio, const std::vector<SphereObj*> spheres)
{
	float xx = (2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
	float yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle;
	Vec3f raydir(xx, yy, -1);
	raydir.normalize();
	*pixel = trace(Vec3f(0), raydir, spheres, 0);
}

void saveSphereImage(ConfigurationSettings configSettings, int iteration, Vec3f* image, unsigned width, unsigned height)
{
	// Save result to a PPM image (keep these flags if you compile under Windows)
	std::stringstream ss;

	ss << configSettings.filePath << "spheres" << iteration << ".ppm";
	std::string tempString = ss.str();
	char* filename = (char*)tempString.c_str();

	std::ofstream ofs(filename, std::ios::out | std::ios::binary);
	ofs << "P6\n" << width << " " << height << "\n255\n";

	for (unsigned i = 0; i < width * height; ++i)
	{
		ofs << (unsigned char)(min(float(1), image[i].x) * 255)
			<< (unsigned char)(min(float(1), image[i].y) * 255)
			<< (unsigned char)(min(float(1), image[i].z) * 255);
	}

	ofs.close();
	delete[] image;
}

std::vector<SphereObj*> RetrieveRootSpheres(std::vector<SphereObj*> spheres)
{
	std::vector<SphereObj*> rootSpheres;

	for each(SphereObj* sphere in spheres)
	{
		bool rootSphere = sphere->GetRootSphere();

		if (rootSphere)
		{
			rootSpheres.push_back(sphere);
		}
	}

	return rootSpheres;
}

//[comment]
// Main rendering function. We compute a camera ray for each pixel of the image
// trace it and return a color. If the ray hits a sphere, we return the color of the
// sphere at the intersection point, else we return the background color.
//[/comment]
void render(std::vector<SphereObj*> spheresToRender, int iteration, ConfigurationSettings configSettings, unsigned width, unsigned height, UINT frameTotal)
{
	std::chrono::time_point<std::chrono::system_clock> frameStart;
	frameStart = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> frameEnd;

	Vec3f* image = new Vec3f[width * height];
	Vec3f* pixel = image;
	float invWidth = 1 / float(width), invHeight = 1 / float(height);
	float fov = 30, aspectratio = width / float(height);
	float angle = tan(M_PI * 0.5f * fov / 180.0f);

	// Trace rays
	for (unsigned y = 0; y < height; y++)
	{
		for (unsigned x = 0; x < width; x++, pixel++)
		{
			renderPixel(pixel, x, y, invWidth, invHeight, angle, aspectratio, spheresToRender);
		}
	}

	// Save result to a PPM image (keep these flags if you compile under Windows)
	saveSphereImage(configSettings, iteration, image, width, height);

	/*std::function<void()> function = std::bind(&saveSphereImage, configSettings, iteration, image, width, height);
	threadManager->AddTask(function);*/

	frameEnd = std::chrono::system_clock::now();
	std::chrono::duration<double> frameDuration = frameEnd - frameStart;

	std::cout << "\nFrame " << iteration + 1 << ": " << frameDuration.count() << "\t| Render Completion: " << iteration + 1 << "/" << frameTotal;
	frameLogFile << "\nFrame " << iteration + 1 << ": " << frameDuration.count() << "\t| Render Completion: " << iteration + 1 << "/" << frameTotal;
}

#pragma region Setup Solar System

ConfigurationSettings ImportSetupFromXMLFile(tinyxml2::XMLDocument &xmlDocument)
{
	ConfigurationSettings configSettings;

	tinyxml2::XMLElement* element = xmlDocument.FirstChildElement()->FirstChildElement();

	configSettings.length				= atoi(element->FirstChildElement("appLength")->GetText());
	configSettings.frameRate			= atoi(element->FirstChildElement("appFrameRate")->GetText());
	configSettings.frameRateSetting		= element->FirstChildElement("appFrameRate")->GetText();
	configSettings.resolutionX			= atoi(element->FirstChildElement("appResolutionX")->GetText());
	configSettings.resolutionY			= atoi(element->FirstChildElement("appResolutionY")->GetText());
	configSettings.resolutionSetting	= element->FirstChildElement("appResolutionCommand")->GetText();

	configSettings.filePath = element->FirstChildElement("appOutputDirectory")->GetText();

	return configSettings;
}

void SortSphereHierarchy(std::vector<SphereObj*> &spheres)
{
	for each(SphereObj* sphere1 in spheres)
	{
		std::string sphere1Name = sphere1->GetSphereName();

		for each(SphereObj* sphere2 in spheres)
		{
			std::string sphere2ParentName = sphere2->GetParentSphereName();

			if (sphere1Name == sphere2ParentName)
			{
				sphere1->AddSphereChild(sphere2);
				sphere2->SetParentSphere(sphere1);
			}
		}
	}
}

std::vector<SphereObj*> ImportSolarSystemFromXML(tinyxml2::XMLDocument &xmlDocument)
{
	std::string sphereName;
	Vec3f center;
	float radius, radius2;
	Vec3f surfaceColor, emissionColor;
	float transparency, reflection;
	float rotationSpeed;

	SphereObj* parentSphere;
	std::vector<SphereObj*> childrenSpheres;

	std::vector<SphereObj*> spheres;

	tinyxml2::XMLElement* element = xmlDocument.FirstChildElement()->FirstChildElement("Spheres");
	tinyxml2::XMLNode* node = element->FirstChild();

	bool isNextSiblingNull = false;

	while (!isNextSiblingNull)
	{
		SphereObj* newSphere = new SphereObj();

		// Set Sphere name
		newSphere->SetSphereName(node->FirstChildElement("name")->GetText());

		// Set Sphere position
		float positionX = (atoi(node->FirstChildElement("positionX")->GetText()) - 550.0f) / 150.0f;
		float positionY = (atoi(node->FirstChildElement("positionY")->GetText()) - 200.0f) / 150.0f;
		newSphere->SetPosition(Vec3f(positionX, positionY, -10.0f));

		// Set Sphere colour
		float colourX = strtof(node->FirstChildElement("colourX")->GetText(), NULL) / 255.0f;
		float colourY = strtof(node->FirstChildElement("colourY")->GetText(), NULL) / 255.0f;
		float colourZ = strtof(node->FirstChildElement("colourZ")->GetText(), NULL) / 255.0f;
		newSphere->SetSurfaceColour(Vec3f(colourX, colourY, colourZ));

		// Set Sphere radius and radius^2
		newSphere->SetRadius(strtof(node->FirstChildElement("radius")->GetText(), NULL) / 400.0f);
		
		// Set Sphere rotation speed
		float rotSpeed = strtof(node->FirstChildElement("rotationSpeed")->GetText(), NULL) / 50.0f;
		newSphere->SetRotationSpeed(rotSpeed);

		// Set Sphere root sphere
		std::string boolTest = node->FirstChildElement("rootSphere")->GetText();
		bool rootSphere = false;

		if (boolTest == "true")
		{
			rootSphere = true;
		}
		else
		{
			rootSphere = false;
		}

		// Temporarily set Emission, Transparency and Reflection to hard-coded values.
		newSphere->SetEmissionColour(0.0f);
		newSphere->SetTransparency(0.5f);
		newSphere->SetReflection(1.0f);

		newSphere->SetRootSphere(rootSphere);

		// Set Parent sphere name if the sphere is not a root sphere
		if (!rootSphere)
		{
			newSphere->SetParentSphereName(node->FirstChildElement("parent")->GetText());
		}

		spheres.push_back(newSphere);

		node = node->NextSibling();

		if (node == NULL)
		{
			isNextSiblingNull = true;
		}
	}

	SortSphereHierarchy(spheres);

	return spheres;
}

#pragma endregion

#pragma region Handle Solar System

std::vector<SphereObj*> RetrieveSphereChildren(SphereObj* rootSphere)
{
	std::vector<SphereObj*> spheres;

	std::vector<SphereObj*> childSpheres = rootSphere->GetChildrenSpheres();
	spheres = childSpheres;

	for each(SphereObj* sphere in childSpheres)
	{
		std::vector<SphereObj*> childrenSpheres = RetrieveSphereChildren(sphere);
		spheres.insert(spheres.end(), childrenSpheres.begin(), childrenSpheres.end());
	}

	return spheres;
}

void PlanetRotation(ConfigurationSettings configSettings, std::vector<SphereObj*> spheresImported, std::ofstream &frameLogFile)
{
	std::cout << "\n";

	// Set Width and Height of generated images
	unsigned width = configSettings.resolutionX, height = configSettings.resolutionY;

	UINT videoLength = configSettings.length;
	UINT FPS = configSettings.frameRate;
	UINT frameTotal = videoLength * FPS;

	int loopIteration;

	for (float r = 0.0f; r <= frameTotal-1; r++)
	{
		std::vector<SphereObj*> spheresToRender;

		float frameIncrement = r / FPS;
		std::vector<SphereObj*> rootSpheres = RetrieveRootSpheres(spheresImported);

		loopIteration = r;

		for each(SphereObj* rootSphere in rootSpheres)
		{
			rootSphere->SetPosition(Vec3f(rootSphere->center.x, rootSphere->center.y, rootSphere->center.z - 0.01f));
			rootSphere->SetSurfaceColour(Vec3f(rootSphere->surfaceColor.x - 0.01f, rootSphere->surfaceColor.y - 0.01f, rootSphere->surfaceColor.z - 0.01f));
			Vec3f rootPos = rootSphere->center;
			rootSphere->UpdateChildren(frameIncrement, rootPos);
		}

		for each (SphereObj* sphere in spheresImported)
		{
			SphereObj* newSphere = new SphereObj();

			newSphere->SetEmissionColour(0.0f);
			newSphere->SetPosition(sphere->GetPosition());
			newSphere->SetRadius(sphere->GetRadius());
			newSphere->SetReflection(1.0f);
			newSphere->SetRootSphere(sphere->GetRootSphere());

			if (!newSphere->GetRootSphere())
			{
				newSphere->SetParentSphereName(sphere->GetParentSphereName());
			}

			newSphere->SetRotationSpeed(sphere->GetRotationSpeed());
			newSphere->SetSphereName(sphere->GetSphereName());
			newSphere->SetSurfaceColour(sphere->GetSurfaceColour());
			newSphere->SetTransparency(0.5f);
			
			spheresToRender.push_back(newSphere);
		}

		std::function<void()> function = std::bind(&render, spheresToRender, loopIteration, configSettings, width, height, frameTotal);
		threadManager->AddTask(function);
	}
}

#pragma endregion


#pragma region Handle Application Output

std::string CreateOutputDirectory(ConfigurationSettings &configSettings)
{
	int filePathCount = 1;

	std::wstring filePathPrefixWS;

	bool filePathNotFound = true;

#ifdef DEBUG

	filePathPrefixWS = L"../Debug/Debug_Application_Output/";
	std::wstring filePath = filePathPrefixWS + L"Application_Output_";

	while (filePathNotFound)
	{
		std::wstring filePathNew = filePath + std::to_wstring(filePathCount);

		int result = GetFileAttributes(filePathNew.c_str());

		if (result != -1)
		{
			filePathCount++;
		}
		else
		{
			filePathNotFound = false;
		}
	}

	filePath += std::to_wstring(filePathCount);
	filePath += L"/";

#else

	filePathCount = 0;

	std::wstring filePath = filePathPrefixWS;
	filePathPrefixWS.assign(configSettings.filePath.begin(), configSettings.filePath.end());

	int result = GetFileAttributes(filePathPrefixWS.c_str());

	if (result != -1)
	{
		std::string filePathPrefixRelease;
		filePathPrefixRelease.assign(filePathPrefixWS.begin(), filePathPrefixWS.end());

		std::string fileRem = filePathPrefixRelease;
		fileRem += "Frame_Log.txt";

		remove(fileRem.c_str());

		fileRem = filePathPrefixRelease;
		fileRem += "video.mp4";

		remove(fileRem.c_str());

		bool filesLeft = true;

		while (filesLeft)
		{
			std::wstring filePathNew = filePathPrefixWS + L"spheres" + std::to_wstring(filePathCount) + L".ppm";

			int result = GetFileAttributes(filePathNew.c_str());

			if (result != -1)
			{
				std::string ppmFilePath = filePathPrefixRelease + "spheres" + std::to_string(filePathCount) + ".ppm";
				remove(ppmFilePath.c_str());

				filePathCount++;
			}
			else
			{
				filesLeft = false;
			}
		}
	}

	filePath += filePathPrefixWS;

#endif

	std::string filePathPrefix;
	filePathPrefix.assign(filePathPrefixWS.begin(), filePathPrefixWS.end());

	CreateDirectory(filePath.c_str(), NULL);

#ifdef DEBUG
	std::string filePathRet = filePathPrefix + "Application_Output_" + std::to_string(filePathCount) + "/";
#else
	std::string filePathRet = filePathPrefix;
#endif

	std::cout << filePathRet;

	return filePathRet;
}

void HandleSolutionConfiguration(ConfigurationSettings &configSettings)
{
#ifdef _DEBUG
	configSettings.frameRateSetting = "10";
	configSettings.frameRate = 10;
	configSettings.resolutionX = 640;
	configSettings.resolutionY = 480;
	configSettings.resolutionSetting = "640x480";
#endif
	configSettings.filePath = CreateOutputDirectory(configSettings);
}

void GenerateVideoFromPPMFiles(ConfigurationSettings configSettings)
{
	std::string ffmpegCommand = "ffmpeg -r " + configSettings.frameRateSetting +" -f image2 -s " + configSettings.resolutionSetting + " -i " + configSettings.filePath + "spheres%d.ppm -vcodec libx264 -crf 25 -pix_fmt yuv420p " + configSettings.filePath + "video.mp4";

	system(ffmpegCommand.c_str());
}

#pragma endregion

#pragma region Handle Frame Log Content

std::string generateFrameLogHeader(ConfigurationSettings configSettings)
{
	std::string frameLogHeader = "";

	frameLogHeader += "Application Configuration Settings:\n";

	frameLogHeader += "Video Length:\t\t";
	frameLogHeader += std::to_string(configSettings.length);
	frameLogHeader += " seconds\n";
	frameLogHeader += "Frames Per Second:\t" + configSettings.frameRateSetting + "\n";
	frameLogHeader += "Resolution:\t\t" + configSettings.resolutionSetting + "\n\n";

	frameLogHeader += "===================================================================\n\n";

	return frameLogHeader;
}

std::string generateFrameLogImportData(std::chrono::duration<double> importDuration, std::time_t importEndTime)
{
	std::string frameLogImportData;

	frameLogImportData += "XML Import Run Time:\t";
	frameLogImportData += std::to_string(importDuration.count());
	frameLogImportData += " seconds | ";
	frameLogImportData += std::to_string((importDuration.count() / 60.0f));
	frameLogImportData += " minutes";
	frameLogImportData += "\nXML Import End Time:\t";
	frameLogImportData += std::ctime(&importEndTime);

	frameLogImportData += "\n\n===================================================================\n";

	return frameLogImportData;
}

std::string generateFrameLogFooter(std::chrono::duration<double> appDuration, std::time_t appEndTime)
{
	std::string frameLogFooter;

	frameLogFooter = "\n\n===================================================================\n\n";

	frameLogFooter += "Render Run Time:\t";
	frameLogFooter += std::to_string(appDuration.count());
	frameLogFooter += " seconds | ";
	frameLogFooter += std::to_string((appDuration.count() / 60.0f));
	frameLogFooter += " minutes";
	frameLogFooter += "\nRender End Time:\t";
	frameLogFooter += std::ctime(&appEndTime);

	return frameLogFooter;
}

#pragma endregion

//[comment]
// In the main function, we will create the scene which is composed of 5 spheres
// and 1 light (which is also a sphere). Then, once the scene description is complete
// we render that scene, by calling the render() function.
//[/comment]
int main(int argc, char **argv)
{
	threadManager = new ThreadManager();

	// This sample only allows one choice per program execution. Feel free to improve upon this
	srand(13);

	// Load XML Solar System Setup file
	tinyxml2::XMLDocument xmlDocument;
	tinyxml2::XMLError error = xmlDocument.LoadFile("../../../XML_Output/XMLOutput.xml");

	if (error == tinyxml2::XML_SUCCESS)
	{
		// Import Configuration Settings and setup file path
		ConfigurationSettings configSettings = ImportSetupFromXMLFile(xmlDocument);
		HandleSolutionConfiguration(configSettings);

		// Open stream in file directory
		frameLogFile.open(configSettings.filePath + "Frame_Log.txt");
		
		// Generate Frame Log Header and stream to file
		std::string frameLogHeader = generateFrameLogHeader(configSettings);
		frameLogFile << frameLogHeader;

		// Begin the Import Clock
		std::chrono::time_point<std::chrono::system_clock> importStart;
		std::chrono::time_point<std::chrono::system_clock> importEnd;
		importStart = std::chrono::system_clock::now();

		// Import Solar System properties from XML file
		std::vector<SphereObj*> spheres = ImportSolarSystemFromXML(xmlDocument);

		// Calculate import duration
		importEnd = std::chrono::system_clock::now();
		std::chrono::duration<double> importDuration = importEnd - importStart;
		std::time_t importEndTime = std::chrono::system_clock::to_time_t(importEnd);

		// Generate Frame Log XML Import Data and stream to file
		std::string frameLogImportData = generateFrameLogImportData(importDuration, importEndTime);
		frameLogFile << frameLogImportData;

		// Begin the Render Clock
		std::chrono::time_point<std::chrono::system_clock> renderStart;
		std::chrono::time_point<std::chrono::system_clock> renderEnd;
		renderStart = std::chrono::system_clock::now();

		// Begin rendering of scene
		PlanetRotation(configSettings, spheres, frameLogFile);

		// Join all threads back to the main thread
		threadManager->JoinAllThreads();

		// Generate the video file from rendered Sphere.ppm files
		GenerateVideoFromPPMFiles(configSettings);

		// Calculate render duration
		renderEnd = std::chrono::system_clock::now();
		std::chrono::duration<double> renderDuration = renderEnd - renderStart;
		std::time_t renderEndTime = std::chrono::system_clock::to_time_t(renderEnd);

		// Generate Frame Log Footer and stream to file
		std::string frameLogFooter = generateFrameLogFooter(renderDuration, renderEndTime);
		frameLogFile << frameLogFooter;

		frameLogFile.close();
	}

	return 0;
}