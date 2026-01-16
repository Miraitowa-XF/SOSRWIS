#include "ParticleSystem.h"
#include <glad/glad.h>
#include <glm/gtc/random.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <glfw/glfw3.h>

#include <fstream>
#include <sstream>
#include "stb_image.h"

//必要参数：重力加速度，
static const float GRAVITY = -0.8f;
static float quad[] = {
	//pos				//tex
	-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
	0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
	0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
	-0.5f,  0.5f, 0.0f, 0.0f, 1.0f
};

unsigned int ParticleSystem::LoadShader(const char* vertPath, const char* fragPath) {
	auto loadFile = [](const char* path) {
		// 1. 二进制模式打开
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open()) {
			std::cout << "Failed to open file: " << path << std::endl;
			return std::string();
		}

		// 2. 检测并跳过 BOM
		unsigned char bom[3] = { 0 };
		file.read(reinterpret_cast<char*>(bom), 3);
		if (!(bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)) {
			file.seekg(0); // 不是 BOM，倒带
		}
		// 是 BOM，直接继续往后读

		std::stringstream ss;
		ss << file.rdbuf();
		return ss.str();
		};

	std::string vertCode = loadFile(vertPath);
	std::string fragCode = loadFile(fragPath);

	const char* vSrc = vertCode.c_str();
	const char* fSrc = fragCode.c_str();

	unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vSrc, nullptr);
	glCompileShader(vs);

	unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fSrc, nullptr);
	glCompileShader(fs);

	unsigned int prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return prog;
}

unsigned int ParticleSystem::LoadTexture(const char* texturePath) {
	stbi_set_flip_vertically_on_load(true);

	int w, h, channels;		//w:width, h:height
	unsigned char* data = stbi_load(texturePath, &w, &h, &channels, 4);
	if (!data) {
		std::cout << "Failed to load texture: " << texturePath << std::endl;
		return 0;
	}
	else {
		printf("Successfully load texture %s (w=%d, h=%d, channels=%d)\n", texturePath, w, h, channels);
	}

	unsigned int texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA,
		w, h, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, data
	);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}


void ParticleSystem::Init(const char* vertPath, const char* fragPath, const char* texturePath) {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));


	shader = LoadShader(
		vertPath,
		fragPath
	);
	textureID = LoadTexture(texturePath);
	locModel = glGetUniformLocation(shader, "model");
	// also cache view/projection uniforms so Render can use them
	locView = glGetUniformLocation(shader, "view");
	locProj = glGetUniformLocation(shader, "projection");

	// set sampler to texture unit 0
	glUseProgram(shader);
	GLint locTex = glGetUniformLocation(shader, "particleTexture");
	if (locTex != -1) glUniform1i(locTex, 0);

	glBindVertexArray(0);
}

void ParticleSystem::SetSpawnRate(float rate) {
	spawnRate = rate;
}

void ParticleSystem::SetWind(const glm::vec3& w) {
	wind = w;
}

void ParticleSystem::SetActive(bool a) {
	active = a;
}

void ParticleSystem::SetTexture(unsigned int texID) {
	textureID = texID;
}

void ParticleSystem::SetShader(unsigned int shaderID) {
	shader = shaderID;
	locView = glGetUniformLocation(shader, "view");
	locProj = glGetUniformLocation(shader, "projection");
	locModel = glGetUniformLocation(shader, "model");

}

void ParticleSystem::SpawnParticle() {
	SnowParticle p;
	//下雪范围: x: -50~50, y: 25~60, z: -50~50
	p.position = glm::vec3(
		glm::linearRand(-50.0f, 50.0f),
		glm::linearRand(25.0f, 80.0f),
		glm::linearRand(-50.0f, 50.0f)
	);
	//初始下落速度
	p.velocity = glm::vec3(
		wind.x + glm::linearRand(-0.2f, 0.2f),
		glm::linearRand(-0.5f, -1.0f),
		wind.z + glm::linearRand(-0.2f, 0.2f)
	);

	p.lifetime = glm::linearRand(8.0f, 18.0f);
	p.size = glm::linearRand(0.1f, 0.2f);

	p.phase = glm::linearRand(0.0f, 6.2831f);
	p.swaySpeed = glm::linearRand(0.5f, 1.5f);
	p.angle = glm::linearRand(0.0f, 6.2831f);
	p.angularSpeed = glm::linearRand(-1.0f, 1.0f);


	particles.push_back(p);
}

void ParticleSystem::Update(float deltaTime, bool smallSnow) {
	if (!active) return;

	spawnAccumulator += deltaTime * spawnRate;	//累加器
	//按速率精确生成新粒子
	while (spawnAccumulator >= 1.0f) {
		SpawnParticle();
		spawnAccumulator -= 1.0f;
	}

	//倒序更新，以便删除粒子
	for (int i = (int)particles.size() - 1; i >= 0; --i) {
		auto& p = particles[i];
		p.lifetime -= deltaTime;
		p.velocity.y += GRAVITY * deltaTime;
		p.position += p.velocity * deltaTime;

		if (p.lifetime <= 0.0f || p.position.y <= -1.0f) {
			std::swap(p, particles.back());
			particles.pop_back();
			//particles.erase(particles.begin() + i);
		}
		else {
			if (!smallSnow) {
				float t = glfwGetTime();
				p.position.x += sin(t * p.swaySpeed + p.phase) * 0.06f; // 0.06f为摆动幅度
				p.position.z += cos(t * p.swaySpeed + p.phase) * 0.03f;
				p.angle += p.angularSpeed * deltaTime;
			}
		}
	}
}

void ParticleSystem::Render(const glm::mat4& view, const glm::mat4& projection) {
	if (!active || particles.empty()) return;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	if (shader == 0) {
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		printf("ParticleSystem::Render: No shader set!\n");
		return;
	}

	glUseProgram(shader);

	// bind particle texture to unit 0 so shader can sample it
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);

	if (locView != -1) glUniformMatrix4fv(locView, 1, GL_FALSE, &view[0][0]);
	else printf("ParticleSystem::Render: 'view' uniform not found in shader!\n");

	if (locProj != -1) glUniformMatrix4fv(locProj, 1, GL_FALSE, &projection[0][0]);
	else printf("ParticleSystem::Render: 'projection' uniform not found in shader!\n");


	glBindVertexArray(VAO);

	for (const auto& p : particles) {
		//构造模型矩阵
		glm::mat4 model(1.0f);
		model = glm::translate(model, p.position);
		model = glm::rotate(model, p.angle, glm::vec3(0, 0, 1));

		//看板逻辑Billboarding
		model[0][0] = view[0][0]; model[0][1] = view[1][0]; model[0][2] = view[2][0];
		model[1][0] = view[0][1]; model[1][1] = view[1][1]; model[1][2] = view[2][1];
		model[2][0] = view[0][2]; model[2][1] = view[1][2]; model[2][2] = view[2][2];

		model = glm::scale(model, glm::vec3(p.size));


		if (locModel != -1) glUniformMatrix4fv(locModel, 1, GL_FALSE, &model[0][0]);
		else printf("ParticleSystem::Render: 'model' uniform not found in shader!\n");

		//四边形绘制
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	glBindVertexArray(0);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}



