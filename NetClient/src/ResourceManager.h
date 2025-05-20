#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H
#include <map>
#include <glad/glad.h>
//#include "rendering/Texture.h"
#include "Rendering/Shader.h"


class ResourceManager
{
public:
	static std::map<std::string, Shader> Shaders;
	//static std::map<std::string, Texture>Textures;

	static Shader LoadShader(const char* vShaderFile, const char* fShaderFile, const char* gShaderFile, std::string name);
	static Shader& GetShader(std::string name);
	//static Texture LoadTexture(const char* file, bool alpha, std::string name);
	//static Texture& GetTexture(std::string name);
	//properly de-allocates all loaded resources
	static void clear();


private:
	ResourceManager() {}


	//static Texture loadTextureFromFIle(const char* File, bool alpha);

};


#endif // !RESOURCEMANAGER_H
