#include <glm/glm.hpp>

#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

const int NO_INTERSECTION(-1.0f);
glm::vec3 BACKGROUND_COLOR(0.0f, 0.5f, 0.5f);
const glm::vec3 UP(0.0f, 1.0f, 0.0f);

struct Ray
{
	glm::vec3 origin;		 // Ray origin
	glm::vec3 direction; // Ray direction
};

struct Material
{
	glm::vec3 ambient;	// Ambient
	glm::vec3 diffuse;	// Diffuse
	glm::vec3 specular; // Specular
	float shininess;		// Shininess
};

struct SceneObject
{
	Material material; // Material

	/**
	 * Template function for calculating the intersection of this object with the provided ray.
	 * @param[in]   incomingRay             Ray that will be checked for intersection with this object
	 * @param[out]  outIntersectionPoint    Point of intersection (in case there is an intersection)
	 * @param[out]  outIntersectionNormal   Normal vector at the point of intersection (in case there is an intersection)
	 * @return If there is an intersection, returns the distance from the ray origin to the intersection point. Otherwise, returns a negative number.
	 */
	virtual float Intersect(const Ray &incomingRay, glm::vec3 &outIntersectionPoint, glm::vec3 &outIntersectionNormal) = 0;
};

// Subclass of SceneObject representing a Sphere scene object
struct Sphere : public SceneObject
{
	glm::vec3 center; // center
	float radius;			// radius

	/**
	 * @brief Ray-sphere intersection
	 * @param[in]   incomingRay             Ray that will be checked for intersection with this object
	 * @param[out]  outIntersectionPoint    Point of intersection (in case there is an intersection)
	 * @param[out]  outIntersectionNormal   Normal vector at the point of intersection (in case there is an intersection)
	 * @return If there is an intersection, returns the distance from the ray origin to the intersection point. Otherwise, returns a negative number.
	 */
	virtual float Intersect(const Ray &incomingRay, glm::vec3 &outIntersectionPoint, glm::vec3 &outIntersectionNormal)
	{
		float s(NO_INTERSECTION);

		// In case there is an intersection, place the intersection point and intersection normal
		// that you calculated to the outIntersectionPoint and outIntersectionNormal variables.
		//
		// When you use this function from the outside, you can pass in the variables by reference.
		//
		// Example:
		// Ray ray = ...;
		// glm::vec3 point, normal;
		// float t = sphere->Intersect(ray, point, normal);
		//
		// (At this point, point and normal will now contain the intersection point and intersection normal)

		// m = P - C
		glm::vec3 m(incomingRay.origin - center);
		float b(glm::dot(m, incomingRay.direction));
		float c(glm::dot(m, m) - (radius * radius));
		float discriminant((b * b) - c);
		float t1, t2;

		// The ray intersects with the sphere once.
		if (discriminant == 0)
			s = -b;

		else if (discriminant > 0)
		{
			t1 = -b + glm::sqrt(discriminant);
			t2 = -b - glm::sqrt(discriminant);

			// The ray starts outside or inside the sphere and intersects it once or twice. Get the smaller and positive root.
			if (!(t1 < 0 and t2 < 0))
				s = (t1 > 0 and t1 < t2) ? t1 : t2;
		}

		if (s != NO_INTERSECTION)
		{
			outIntersectionPoint = incomingRay.origin + (s * incomingRay.direction);
			outIntersectionNormal = glm::normalize(outIntersectionPoint - center);
		}
		return s;
	}
};

// Subclass of SceneObject representing a Triangle scene object
struct Triangle : public SceneObject
{
	glm::vec3 A; // First point
	glm::vec3 B; // Second point
	glm::vec3 C; // Third point

	/**
	 * @brief Ray-Triangle intersection
	 * @param[in]   incomingRay             Ray that will be checked for intersection with this object
	 * @param[out]  outIntersectionPoint    Point of intersection (in case there is an intersection)
	 * @param[out]  outIntersectionNormal   Normal vector at the point of intersection (in case there is an intersection)
	 * @return If there is an intersection, returns the distance from the ray origin to the intersection point. Otherwise, returns a negative number.
	 */
	virtual float Intersect(const Ray &incomingRay, glm::vec3 &outIntersectionPoint, glm::vec3 &outIntersectionNormal)
	{
		float s(NO_INTERSECTION);

		glm::vec3 n(glm::cross(B - A, C - A));
		glm::vec3 e(glm::cross(-incomingRay.direction, incomingRay.origin - A));
		float f(glm::dot(-incomingRay.direction, n));

		float t(glm::dot((incomingRay.origin - A), n) / f);
		float u(glm::dot(C - A, e) / f);
		float v(-glm::dot(B - A, e) / f);
		
		if (u >= 0 and v >= 0 and u + v <= 1)
		{
			s = t;
		}


		if (s != NO_INTERSECTION)
		{
			outIntersectionPoint = incomingRay.origin + (t * incomingRay.direction);
			outIntersectionNormal = glm::normalize(n);
		}
		return s;
	}
};

struct Camera
{
	glm::vec3 position;		// Position
	glm::vec3 lookTarget; // Look target
	glm::vec3 globalUp;		// Global up-vector
	float fovY;						// Vertical field of view
	float focalLength;		// Focal length

	int imageWidth;	 // image width
	int imageHeight; // image height
};

struct Light
{
	glm::vec4 position; // Light position (w = 1 if point light, w = 0 if directional light)

	glm::vec3 ambient;	// Light's ambient intensity
	glm::vec3 diffuse;	// Light's diffuse intensity
	glm::vec3 specular; // Light's specular intensity

	// --- Attenuation variables ---
	float constant;	 // Constant factor
	float linear;		 // Linear factor
	float quadratic; // Quadratic factor
};

struct IntersectionInfo
{
	Ray incomingRay;							// Ray used to calculate the intersection
	float t;											// Distance from the ray's origin to the point of intersection (if there was an intersection).
	SceneObject *obj;							// Object that the ray intersected with. If this is equal to nullptr, then no intersection occured.
	glm::vec3 intersectionPoint;	// Point where the intersection occured (if there was an intersection)
	glm::vec3 intersectionNormal; // Normal vector at the point of intersection (if there was an intersection)
};

struct Scene
{
	std::vector<SceneObject *> objects; // List of all objects in the scene
	std::vector<Light> lights;					// List of all lights in the scene
};

struct Image
{
	std::vector<unsigned char> data; // Image data
	int width;											 // Image width
	int height;											 // Image height

	/**
	 * @brief Constructor
	 * @param[in] w Width
	 * @param[in] h Height
	 */
	Image(const int &w, const int &h)
			: width(w), height(h)
	{
		data.resize(w * h * 3, 0);
	}

	/**
	 * @brief Converts the provided color value from [0, 1] to [0, 255]
	 * @param[in] c Color value in [0, 1] range
	 * @return Color value in [0, 255] range
	 */
	unsigned char ToChar(float c)
	{
		c = glm::clamp(c, 0.0f, 1.0f);
		return static_cast<unsigned char>(c * 255);
	}

	/**
	 * @brief Sets the color at the specified pixel location
	 * @param[in] x     X-coordinate of the pixel
	 * @param[in] y     Y-coordinate of the pixel
	 * @param[in] color Pixel color
	 */
	void SetColor(const int &x, const int &y, const glm::vec3 &color)
	{
		int index = (y * width + x) * 3;
		data[index] = ToChar(color.r);
		data[index + 1] = ToChar(color.g);
		data[index + 2] = ToChar(color.b);
	}
};

/**
 * @brief Gets the ray that goes from the camera's position to the specified pixel at (x, y)
 * @param[in] camera Camera data
 * @param[in] x X-coordinate of the pixel (upper-left corner of the pixel)
 * @param[in] y Y-coordinate of the pixel (upper-left corner of the pixel)
 * @return Ray that passes through the pixel at (x, y)
 */
Ray GetRayThruPixel(const Camera &camera, const int &pixelX, const int &pixelY)
{
	Ray ray;
	glm::vec3 cameraLookDirection(glm::normalize(camera.lookTarget - camera.position));

	float viewportHeight(2 * camera.focalLength * glm::tan(glm::radians(camera.fovY) / 2));
	float viewportWidth(camera.imageWidth * viewportHeight / camera.imageHeight);

	glm::vec3 u(glm::normalize(glm::cross(cameraLookDirection, UP)));
	glm::vec3 v(glm::normalize(glm::cross(u, cameraLookDirection)));

	glm::vec3 viewportLowerLeft(camera.position + (cameraLookDirection * camera.focalLength) - (u * (viewportWidth / 2)) - (v * (viewportHeight / 2)));

	float pixelXOffset(0.5f); // the part of the pixel that the ray passes through
	float pixelYOffset(0.5f);
	float s((pixelX + pixelXOffset) * viewportWidth / camera.imageWidth);
	float t((pixelY + pixelYOffset) * viewportHeight / camera.imageHeight);

	glm::vec3 pixelPosition(viewportLowerLeft + (u * s) + (v * t));

	ray.origin = camera.position;
	ray.direction = glm::normalize(pixelPosition - ray.origin);

	return ray;
}

/**
 * @brief Cast a ray to the scene.
 * @param[in] ray   Ray to cast to the scene
 * @param[in] scene Scene object
 * @return Returns an IntersectionInfo object that will contain the results of the raycast
 */
IntersectionInfo Raycast(const Ray &ray, const Scene &scene)
{
	IntersectionInfo ret;
	float tTemp;
	ret.incomingRay = ray;

	// Go through all objects in the scene.
	// If the object is closer to the ray origin than the last object, overwrite the value of ret.t.
	for (size_t i = 0; i < scene.objects.size(); ++i)
	{
		tTemp = scene.objects[i]->Intersect(ray, ret.intersectionPoint, ret.intersectionNormal);

		if (i == 0)
		{
			ret.t = tTemp;
			ret.obj = (tTemp == NO_INTERSECTION) ? nullptr : scene.objects[i];
		}
		else
		{
			if ((tTemp > 0 and ret.t > 0 and tTemp < ret.t) or (ret.t == NO_INTERSECTION and tTemp > 0))
			{
				ret.t = tTemp;
				ret.obj = scene.objects[i];
			}
		}
	}

	return ret;
}

/**
 * @brief Perform a ray-trace to the scene
 * @param[in] ray       Ray to trace
 * @param[in] scene     Scene data
 * @param[in] camera    Camera data
 * @param[in] maxDepth  Maximum depth of the trace
 * @return Resulting color after the ray bounced around the scene
 */
glm::vec3 RayTrace(const Ray &ray, const Scene &scene, const Camera &camera, int maxDepth = 1)
{
	glm::vec3 color = BACKGROUND_COLOR;

	IntersectionInfo intersectionInfo = Raycast(ray, scene);
	if (intersectionInfo.obj != nullptr)
	{
		color = intersectionInfo.obj->material.diffuse;
	}

	return color;
}

/**
 * Main function
 */
int main()
{
	Scene scene;
	int numOfObjects, numOfLights;
	Camera camera;
	int maxDepth;

	// camera.position = glm::vec3(0.0f, 0.0f, 3.0f);
	// camera.lookTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	// camera.globalUp = glm::vec3(0.0f, 1.0f, 0.0f);
	// camera.fovY = 45.0f;
	// camera.focalLength = 1.0f;
	// camera.imageWidth = 640;
	// camera.imageHeight = 480;

	// open .test file
	std::ifstream sceneFile;
	std::string filename;
	std::cout << "Enter filename inside ./test directory: ";
	std::cin >> filename;
	sceneFile.open("./test/" + filename);

	if (!sceneFile)
	{
		std::cerr << "File not found.\n";
		exit(1);
	}

	// camera data
	sceneFile >> camera.imageWidth >> camera.imageHeight;
	sceneFile >> camera.position.x >> camera.position.y >> camera.position.z;
	sceneFile >> camera.lookTarget.x >> camera.lookTarget.y >> camera.lookTarget.z;
	sceneFile >> camera.globalUp.x >> camera.globalUp.y >> camera.globalUp.z;
	sceneFile >> camera.fovY >> camera.focalLength;

	sceneFile >> maxDepth >> numOfObjects;

	std::string objectType;
	Sphere *sphere;
	Triangle *triangle;
	Material material;
	for (size_t i = 0; i < numOfObjects; ++i)
	{
		sceneFile >> objectType;
		if (objectType == "sphere") // SPHERE
		{
			sphere = new Sphere();
			sceneFile >> sphere->center.x >> sphere->center.y >> sphere->center.z >> sphere->radius;
		}
		else // TRIANGLE
		{
			triangle = new Triangle();
			sceneFile >> triangle->A.x >> triangle->A.y >> triangle->A.z;
			sceneFile >> triangle->B.x >> triangle->B.y >> triangle->B.z;
			sceneFile >> triangle->C.x >> triangle->C.y >> triangle->C.z;
		}

		sceneFile >> material.ambient.r >> material.ambient.g >> material.ambient.b;
		sceneFile >> material.diffuse.r >> material.diffuse.g >> material.diffuse.b;
		sceneFile >> material.specular.r >> material.specular.g >> material.specular.b;
		sceneFile >> material.shininess;
		if (objectType == "sphere")
		{
			sphere->material = material;
			scene.objects.push_back(sphere);
		}
		else
		{
			triangle->material = material;
			scene.objects.push_back(triangle);
		}
	}

	sceneFile >> numOfLights;

	Light light;
	for (size_t i = 0; i < numOfLights; ++i)
	{
		sceneFile >> light.position.x >> light.position.y >> light.position.z >> light.position.w;
		sceneFile >> light.ambient.r >> light.ambient.g >> light.ambient.b;
		sceneFile >> light.diffuse.r >> light.diffuse.g >> light.diffuse.b;
		sceneFile >> light.specular.r >> light.specular.g >> light.specular.b;
		sceneFile >> light.constant >> light.linear >> light.quadratic;
	}

	// triangle = new Triangle();
	// triangle->A = glm::vec3(1.5f, -1.5f, 0.0f);
	// triangle->B = glm::vec3(0.0f, 1.5f, 0.0f);
	// triangle->C = glm::vec3(-1.5f, -1.5f, 0.0f);
	// triangle->material.diffuse = glm::vec3(1, 0, 1);
	// scene.objects.push_back(triangle);

	// for each pixel in viewport, cast a ray and set the calculated color to the corresponding pixel
	Image image(camera.imageWidth, camera.imageHeight);
	for (int y = 0; y < image.height; ++y)
	{
		for (int x = 0; x < image.width; ++x)
		{
			Ray ray = GetRayThruPixel(camera, x, image.height - y - 1);

			glm::vec3 color = RayTrace(ray, scene, camera, maxDepth);
			image.SetColor(x, y, color);
		}

		std::cout << "Row: " << std::setfill(' ') << std::setw(4) << (y + 1) << " / " << std::setfill(' ') << std::setw(4) << image.height << "\r" << std::flush;
	}
	std::cout << std::endl;

	std::string imageFileName = "scene.png"; // You might need to make this a full path if you are on Mac
	stbi_write_png(imageFileName.c_str(), image.width, image.height, 3, image.data.data(), 0);

	for (size_t i = 0; i < scene.objects.size(); ++i)
	{
		delete scene.objects[i];
	}

	// DEBUG only
	// system("pause");

	return 0;
}
