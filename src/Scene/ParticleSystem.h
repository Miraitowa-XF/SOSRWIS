#include<vector>
#include<glm/glm.hpp>
#include<glad/glad.h> 

/*
参数说明：

SnowParticle: 单个雪花粒子的数据结构
其中：
	- position: 粒子位置
	- velocity: 粒子速度
	- lifetime: 粒子存活时间
	- size: 粒子大小

ParticleSystem: 粒子系统类，管理雪花粒子的生成、更新和渲染
其中：
	- spawnRate: 生成粒子的速率（每秒多少个）
	- wind: 风的影响向量
	- active: 粒子系统是否激活
	- particles: 存储所有活跃粒子的容器
	- spawnAccumulator: 用于按速率生成粒子的累加器

要修改SnowParticle的属性范围，请到ParticleSystem.cpp中的ParticleSystem::SpawnParticle()函数中修改粒子生成时的随机范围。


要修改PatrticleSystem初始化的参数，请到Scene.cpp中的SnowScene::Init函数中修改粒子系统的相关设置。
主要是涉及到：
		void SetSpawnRate(float rate);
		void SetWind(const glm::vec3& wind);
		void SetActive(bool active);

综上：即到：ParticleSystem.cpp\ParticleSystem::SpawnParticle()、Scene.cpp\SnowScene::Init
*/


struct SnowParticle {
	glm::vec3 position;
	glm::vec3 velocity;
	float lifetime;
	float size;
	SnowParticle()
		: position(0.0f), velocity(0.0f), lifetime(0.0f), size(1.0f) {
	}
	SnowParticle(const glm::vec3& pos, const glm::vec3& vel, float life, float s)
		: position(pos), velocity(vel), lifetime(life), size(s) {
	}
};

class ParticleSystem {
	public:
		void Init(const char* vertPath, const char* fragPath, const char* texturePath);
		void Update(float deltaTime);
		void Render(const glm::mat4& view, const glm::mat4& proj);

		void SetSpawnRate(float rate);
		void SetWind(const glm::vec3& wind);
		void SetActive(bool active);

		void SetTexture(unsigned int texID);
		void SetShader(unsigned int shaderID);

	private:
		void SpawnParticle();
		unsigned int LoadShader(const char* vertPath, const char* fragPath);
		unsigned int LoadTexture(const char* texturePath);


		std::vector<SnowParticle> particles;
		float spawnRate = 10.0f; // particles per second
		float spawnAccumulator = 0.0f;
		glm::vec3 wind = glm::vec3(0.2f, 0.0f, 0.1f);
		bool active = false;

		//渲染相关
		unsigned int VAO = 0, VBO = 0;
		unsigned int shader = 0;
		unsigned int textureID = 0;

		GLint locView = -1;
		GLint locProj = -1;
		GLint locModel = -1;
};

