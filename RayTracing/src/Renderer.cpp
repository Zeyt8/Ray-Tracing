#include "Renderer.h"
#include "Walnut/Random.h"

namespace Utils
{
	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
		return result;
	}
}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
		{
			return;
		}
		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;

	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
		{
			glm::vec4 color = PerPixel(x, y);
			color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(color);
		}
	}

	m_FinalImage->SetData(m_ImageData);
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];

	glm::vec3 color(0);
	float multiplier = 1;

	int bounces = 2;
	for (int i = 0; i < bounces; i++)
	{
		Renderer::HitPayload hitPayload = TraceRay(ray);
		if (hitPayload.HitDistance < 0)
		{
			glm::vec3 skyColor = glm::vec3(0, 0, 0);
			color += skyColor * multiplier;

			break;
		}

		glm::vec3 lightDir = glm::normalize(glm::vec3(-1, -1, -1));
		float lightIntensity = glm::max(glm::dot(hitPayload.WorldNormal, -lightDir), 0.0f);

		const Sphere& sphere = m_ActiveScene->Spheres[hitPayload.ObjectIndex];
		glm::vec3 sphereColor = sphere.Albedo;
		sphereColor *= lightIntensity;
		color += sphereColor* multiplier;

		multiplier *= 0.7f;

		ray.Origin = hitPayload.WorldPosition + hitPayload.WorldNormal * 0.0001f;
		ray.Direction = glm::reflect(ray.Direction, hitPayload.WorldNormal);
	}

	return glm::vec4(color, 1);
}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray)
{
	int closestSphere = -1;
	float hitDistance = std::numeric_limits<float>::max();

	for (size_t i = 0; i < m_ActiveScene->Spheres.size(); i++)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];
		glm::vec3 origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		float discriminant = b * b - 4.0f * a * c;
		if (discriminant < 0)
		{
			continue;
		}

		float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
		//float t0 = (-b + glm::sqrt(discriminant)) / (2.0f * a);
		if (closestT > 0 && closestT < hitDistance)
		{
			hitDistance = closestT;
			closestSphere = (int)i;
		}
	}
	
	if (closestSphere < 0)
	{
		return Miss(ray);
	}

	return ClosestHit(ray, hitDistance, closestSphere);
}

Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, int objectIndex)
{
	Renderer::HitPayload hitPayload;
	hitPayload.HitDistance = hitDistance;
	hitPayload.ObjectIndex = objectIndex;

	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

	glm::vec3 origin = ray.Origin - closestSphere.Position;
	hitPayload.WorldPosition = origin + ray.Direction * hitDistance;
	hitPayload.WorldNormal = glm::normalize(hitPayload.WorldPosition);

	hitPayload.WorldPosition += closestSphere.Position;

	return hitPayload;
}

Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayload hitPayload;
	hitPayload.HitDistance = -1;

	return hitPayload;
}