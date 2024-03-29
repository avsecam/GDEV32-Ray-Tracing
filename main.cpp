#include <glm/glm.hpp>

#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

enum LightType
{
	DIRECTIONAL_LIGHT,
	POINT_LIGHT
};

const int NO_INTERSECTION(-1.0f);
const glm::vec3 BACKGROUND_COLOR(0.0f, 0.0f, 0.0f);
const glm::vec3 UP(0.0f, 1.0f, 0.0f);
const float SHADOW_BIAS(0.001f);
const float REFLECTION_BIAS(0.001f);
const float REFLECTIVITY_CONSTANT(128.0f);
const int SAMPLES_PER_PIXEL(5);

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
	virtual float Intersect(const Ray& incomingRay, glm::vec3& outIntersectionPoint, glm::vec3& outIntersectionNormal) = 0;
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
	virtual float Intersect(const Ray& incomingRay, glm::vec3& outIntersectionPoint, glm::vec3& outIntersectionNormal)
	{
		float s(NO_INTERSECTION);

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
			if (t1 > 0 or t2 > 0)
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
	virtual float Intersect(const Ray& incomingRay, glm::vec3& outIntersectionPoint, glm::vec3& outIntersectionNormal)
	{
		float s(NO_INTERSECTION);

		glm::vec3 n(glm::cross(B - A, C - A));
		glm::vec3 e(glm::cross(-incomingRay.direction, incomingRay.origin - A));
		float f(glm::dot(-incomingRay.direction, n));

		float t(glm::dot((incomingRay.origin - A), n) / f);
		float u(glm::dot(C - A, e) / f);
		float v(-glm::dot(B - A, e) / f);

		if (f > 0 and t > 0 and u >= 0 and v >= 0 and u + v <= 1)
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
	SceneObject* obj;							// Object that the ray intersected with. If this is equal to nullptr, then no intersection occured.
	glm::vec3 intersectionPoint;	// Point where the intersection occured (if there was an intersection)
	glm::vec3 intersectionNormal; // Normal vector at the point of intersection (if there was an intersection)
};

struct Scene
{
	std::vector<SceneObject*> objects; // List of all objects in the scene
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
	Image(const int& w, const int& h)
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
	void SetColor(const int& x, const int& y, const glm::vec3& color)
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
 * @param[in] aa anti-aliasing
 * @return Ray that passes through the pixel at (x, y)
 */
Ray GetRayThruPixel(const Camera& camera, const int& pixelX, const int& pixelY, const bool aa = false)
{
	Ray ray;
	glm::vec3 cameraLookDirection(glm::normalize(camera.lookTarget - camera.position));

	// VIEWPORT SIZING
	float viewportHeight(2 * camera.focalLength * glm::tan(glm::radians(camera.fovY) / 2));
	float viewportWidth(camera.imageWidth * viewportHeight / camera.imageHeight);

	// UV directions of the camera
	glm::vec3 u(glm::normalize(glm::cross(cameraLookDirection, camera.globalUp)));
	glm::vec3 v(glm::normalize(glm::cross(u, cameraLookDirection)));

	glm::vec3 viewportLowerLeft(camera.position + (cameraLookDirection * camera.focalLength) - (u * (viewportWidth / 2)) - (v * (viewportHeight / 2)));

	float pixelXOffset(0.5f); // the part of the pixel that the ray passes through
	float pixelYOffset(0.5f);
	if (aa)
	{
		pixelXOffset = static_cast<float>(rand()) / static_cast<float>(RAND_MAX); // the part of the pixel that the ray passes through
		pixelYOffset = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	}

	// position of pixel in viewport
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
IntersectionInfo Raycast(const Ray& ray, const Scene& scene)
{
	IntersectionInfo ret;
	IntersectionInfo infoTemp;
	ret.incomingRay = ray;
	infoTemp.incomingRay = ray;
	infoTemp.intersectionPoint = glm::vec3();
	infoTemp.intersectionNormal = glm::vec3();

	// Go through all objects in the scene.
	// If the object is closer to the ray origin than the last object, overwrite the contents of ret.
	for (size_t i = 0; i < scene.objects.size(); ++i)
	{
		infoTemp.t = scene.objects[i]->Intersect(infoTemp.incomingRay, infoTemp.intersectionPoint, infoTemp.intersectionNormal);

		// Set ret.t on first iteration
		// Only set obj, point, and normal if infoTemp.t is an intersection
		if (i == 0)
		{
			ret.t = infoTemp.t;
			if (infoTemp.t != NO_INTERSECTION)
			{
				ret.obj = scene.objects[i];
				ret.intersectionPoint = infoTemp.intersectionPoint;
				ret.intersectionNormal = infoTemp.intersectionNormal;
			}
			else
			{
				ret.obj = nullptr;
			}
		}
		// On succeeding iterations, only overwrite ret if:
		// both infoTemp and ret's t values are positive, and infoTemp.t is less than ret.t, OR
		// ret.t has no intersection while infoTemp.t has
		else
		{
			if ((infoTemp.t > 0 and ret.t > 0 and infoTemp.t < ret.t) or (ret.t == NO_INTERSECTION and infoTemp.t > 0))
			{
				ret.t = infoTemp.t;
				ret.obj = scene.objects[i];
				ret.intersectionPoint = infoTemp.intersectionPoint;
				ret.intersectionNormal = infoTemp.intersectionNormal;
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
glm::vec3 RayTrace(const Ray& ray, const Scene& scene, const Camera& camera, int maxDepth = 1)
{
	glm::vec3 color(BACKGROUND_COLOR);

	glm::vec3 ambient, diffuse, specular;
	glm::vec3 directionToLight;
	float distanceToLight;
	float diffuseStrength;
	glm::vec3 reflectedLight;
	float specularStrength;
	float attenuation;

	Ray shadowRay;
	IntersectionInfo shadowingInfo;

	Ray reflectionRay;

	IntersectionInfo intersectionInfo = Raycast(ray, scene);
	if (intersectionInfo.obj != nullptr)
	{
		// color = glm::vec3();
		for (size_t i = 0; i < scene.lights.size(); ++i)
		{
			// AMBIENT
			ambient = intersectionInfo.obj->material.ambient * (scene.lights[i].ambient / static_cast<float>(scene.lights.size()));

			// DIFFUSE
			directionToLight = (scene.lights[i].position.w == POINT_LIGHT)
				? glm::normalize(glm::vec3(scene.lights[i].position) - intersectionInfo.intersectionPoint)
				: glm::normalize(glm::vec3(-scene.lights[i].position));
			diffuseStrength = glm::max(glm::dot(directionToLight, intersectionInfo.intersectionNormal), 0.0f);
			diffuse = diffuseStrength * intersectionInfo.obj->material.diffuse * scene.lights[i].diffuse;

			// SPECULAR
			reflectedLight = glm::reflect(-directionToLight, intersectionInfo.intersectionNormal);
			specularStrength = glm::pow(glm::max(glm::dot(reflectedLight, glm::normalize(camera.position - intersectionInfo.intersectionPoint)), 0.0f), intersectionInfo.obj->material.shininess);
			specular = specularStrength * intersectionInfo.obj->material.specular * scene.lights[i].specular;

			// ATTENUATION
			attenuation = 1.0f;

			if (scene.lights[i].position.w != DIRECTIONAL_LIGHT)
			{
				distanceToLight = glm::distance(intersectionInfo.intersectionPoint, glm::vec3(scene.lights[i].position));
				attenuation = 1.0f / (scene.lights[i].constant + (scene.lights[i].linear * distanceToLight) + (scene.lights[i].quadratic * distanceToLight * distanceToLight));
			}

			// SHADOWING
			shadowRay.origin = intersectionInfo.intersectionPoint + (intersectionInfo.intersectionNormal * SHADOW_BIAS);
			shadowRay.direction = directionToLight;
			shadowingInfo = Raycast(shadowRay, scene);

			color += ambient;

			distanceToLight = (scene.lights[i].position.w == POINT_LIGHT)
				? glm::distance(shadowRay.origin, glm::vec3(scene.lights[i].position))
				: glm::distance(shadowRay.origin, shadowRay.direction * 999.0f);
			
			// Lit when (.obj is null) or (.obj is not null but distance to intersectionPoint is greater than distance to light)
			if ((shadowingInfo.obj == nullptr) or ((shadowingInfo.obj != nullptr) and (glm::distance(shadowRay.origin, shadowingInfo.intersectionPoint) > distanceToLight)))
			{
				color += (diffuse + specular) * attenuation;

				// REFLECTION
				if (maxDepth > 1)
				{
					reflectionRay.origin = intersectionInfo.intersectionPoint + (intersectionInfo.intersectionNormal * REFLECTION_BIAS);
					reflectionRay.direction = glm::reflect(intersectionInfo.incomingRay.direction, intersectionInfo.intersectionNormal);

					color += RayTrace(reflectionRay, scene, camera, maxDepth - 1) * intersectionInfo.obj->material.shininess / REFLECTIVITY_CONSTANT;
				}
			}
		}
	}

	return color;
}

/**
 * Main function
 */
int main()
{
	char antiAliasingChoice;
	bool antiAliasing(false);

	Scene scene;
	int numOfObjects, numOfLights;
	Camera camera;
	int maxDepth;

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
	Sphere* sphere;
	Triangle* triangle;
	Material material;
	sphere = new Sphere();
	triangle = new Triangle();
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

	Light* light;
	for (size_t i = 0; i < numOfLights; ++i)
	{
		light = new Light();
		sceneFile >> light->position.x >> light->position.y >> light->position.z >> light->position.w;
		sceneFile >> light->ambient.r >> light->ambient.g >> light->ambient.b;
		sceneFile >> light->diffuse.r >> light->diffuse.g >> light->diffuse.b;
		sceneFile >> light->specular.r >> light->specular.g >> light->specular.b;
		sceneFile >> light->constant >> light->linear >> light->quadratic;
		scene.lights.push_back(*light);
	}

	std::cout << "Enable anti-aliasing? (Y/N) ";
	std::cin >> antiAliasingChoice;
	if (tolower(antiAliasingChoice) == 'y')
		antiAliasing = true;

	// for each pixel in viewport, cast a ray and set the calculated color to the corresponding pixel
	Image image(camera.imageWidth, camera.imageHeight);
	for (int y = 0; y < image.height; ++y)
	{
		for (int x = 0; x < image.width; ++x)
		{
			// ANTI-ALIASING
			if (antiAliasing)
			{
				glm::vec3 colorSum;
				for (int i = 0; i < SAMPLES_PER_PIXEL; ++i)
				{
					Ray ray = GetRayThruPixel(camera, x, image.height - y - 1, antiAliasing);
					colorSum += RayTrace(ray, scene, camera, maxDepth);
				}
				colorSum /= SAMPLES_PER_PIXEL;
				image.SetColor(x, y, colorSum);
			}
			else
			{
				Ray ray(GetRayThruPixel(camera, x, image.height - y - 1));
				glm::vec3 color(RayTrace(ray, scene, camera, maxDepth));
				image.SetColor(x, y, color);
			}
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