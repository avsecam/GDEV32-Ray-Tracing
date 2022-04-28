#include <glm/glm.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

struct Ray
{
    glm::vec3 origin; // Ray origin
    glm::vec3 direction; // Ray direction
};

struct Material
{
    glm::vec3 ambient; // Ambient
    glm::vec3 diffuse; // Diffuse
    glm::vec3 specular; // Specular
    float shininess; // Shininess
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
    float radius; // radius

    /**
     * @brief Ray-sphere intersection
     * @param[in]   incomingRay             Ray that will be checked for intersection with this object
     * @param[out]  outIntersectionPoint    Point of intersection (in case there is an intersection)
     * @param[out]  outIntersectionNormal   Normal vector at the point of intersection (in case there is an intersection)
     * @return If there is an intersection, returns the distance from the ray origin to the intersection point. Otherwise, returns a negative number.
     */
    virtual float Intersect(const Ray& incomingRay, glm::vec3& outIntersectionPoint, glm::vec3& outIntersectionNormal)
    {
        float s = 0.0f;

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
        float s = 0.0f;

        // The same idea for the outIntersectionPoint and outIntersectionNormal applies here

	    return s;
    }
};

struct Camera
{
    glm::vec3 position; // Position
    glm::vec3 lookTarget; // Look target
    glm::vec3 globalUp; // Global up-vector
    float fovY; // Vertical field of view
    float focalLength; // Focal length

    int imageWidth; // image width
    int imageHeight; // image height
};

struct Light
{
    glm::vec4 position; // Light position (w = 1 if point light, w = 0 if directional light)

    glm::vec3 ambient; // Light's ambient intensity
    glm::vec3 diffuse; // Light's diffuse intensity
    glm::vec3 specular; // Light's specular intensity

    // --- Attenuation variables ---
    float constant; // Constant factor
    float linear; // Linear factor
    float quadratic; // Quadratic factor
};

struct IntersectionInfo
{
    Ray incomingRay; // Ray used to calculate the intersection
    float t; // Distance from the ray's origin to the point of intersection (if there was an intersection).
    SceneObject* obj; // Object that the ray intersected with. If this is equal to nullptr, then no intersection occured.
    glm::vec3 intersectionPoint; // Point where the intersection occured (if there was an intersection)
    glm::vec3 intersectionNormal; // Normal vector at the point of intersection (if there was an intersection)
};

struct Scene
{
    std::vector<SceneObject*> objects; // List of all objects in the scene
    std::vector<Light> lights; // List of all lights in the scene
};

struct Image
{
    std::vector<unsigned char> data; // Image data
    int width; // Image width
    int height; // Image height

    /**
     * @brief Constructor
     * @param[in] w Width
     * @param[in] h Height
     */
    Image(const int& w, const int& h)
        : width(w)
        , height(h)
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
 * @return Ray that passes through the pixel at (x, y)
 */
Ray GetRayThruPixel(const Camera &camera, const int& pixelX, const int& pixelY)
{
    Ray ray;
    ray.origin = glm::vec3(0.0f); 
    ray.direction = glm::vec3(0.0f);

    return ray;
}

/**
 * @brief Cast a ray to the scene.
 * @param[in] ray   Ray to cast to the scene
 * @param[in] scene Scene object
 * @return Returns an IntersectionInfo object that will contain the results of the raycast
 */
IntersectionInfo Raycast(const Ray& ray, const Scene &scene)
{
    IntersectionInfo ret;
    ret.incomingRay = ray;

    // Fields that need to be populated:
    ret.intersectionPoint = glm::vec3(0.0f); // Intersection point
    ret.intersectionNormal = glm::vec3(0.0f); // Intersection normal
    ret.t = 0.0f; // Distance from ray origin to intersection point
    ret.obj = nullptr; // First object hit by the ray. Set to nullptr if the ray does not hit anything

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
    glm::vec3 color(0.0f);
    return color;
}

/**
 * Main function
 */
int main()
{
    Scene scene;
    Camera camera;
    camera.imageWidth = 640;
    camera.imageHeight = 480;
    int maxDepth = 1;

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

    return 0;
}

