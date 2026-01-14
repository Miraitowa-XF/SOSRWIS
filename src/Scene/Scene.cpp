#include "Scene.h"
#include "Core/Camera.h"

void SnowScene::Init(const char* vertPath, const char* fragPath, const char* texturePath) {
	//Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
	//camera = Camera(glm::vec3(0.0f, 3.0f, 6.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);

	particleSystem.Init(vertPath, fragPath, texturePath);
	particleSystem.SetActive(false);
	particleSystem.SetSpawnRate(1000.0f);
	particleSystem.SetWind(glm::vec3(0.24f, 0.0f, 0.16f));
}

void SnowScene::Update(float deltaTime) {
	//camera更新
	//camera.Update(deltaTime);
	//粒子更新
	particleSystem.Update(deltaTime);
}

void SnowScene::Render(Camera camera) {
	//获取视图矩阵和投影矩阵
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 projection = glm::perspective(
		glm::radians(camera.Zoom),
		1280.0f / 720.0f,
		0.1f,
		100.0f
	);
	//渲染粒子系统
	particleSystem.Render(view, projection);
}

ParticleSystem& SnowScene::GetParticleSystem() {
	return particleSystem;
}