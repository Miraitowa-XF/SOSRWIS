#include "ParticleSystem.h"
#include "Core/Camera.h"

// 下雪场景类
class SnowScene {
public:
	void Init(const char* vertPath, const char* fragPath, const char* texturePath);
	void Update(float deltaTime);
	void Render(Camera camera);
	void setSmallSnow(bool set);
	ParticleSystem& GetParticleSystem();

private:
	ParticleSystem particleSystem;
	bool smallSnow = false;
	//Camera camera;
};