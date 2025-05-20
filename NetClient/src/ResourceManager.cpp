#include "ResourceManager.h"
#include <iostream>
std::map<std::string, Shader>ResourceManager::Shaders;

Shader ResourceManager::LoadShader(const char* vShaderFile, const char* fShaderFile, const char* gShaderFile, std::string name)
{

    Shader shader(vShaderFile, fShaderFile, gShaderFile);
    Shaders[name] = shader;
    return Shaders[name] = shader;
}

Shader& ResourceManager::GetShader(std::string name)
{
    return Shaders[name];
}

void ResourceManager::clear()
{
    Shaders.clear();
}
