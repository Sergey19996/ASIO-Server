#include "ResourceManager.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <sstream> //предоставляет классы для работы со строками как с потоками ввода-вывода.
#include <fstream> // для чтения/записи файлов

//Instantiate static variables
std::map<std::string, Texture2D> ResourceManager::Textures;
std::map<ShaderID, Shader> ResourceManager::Shaders;
std::unordered_map<std::string, FontAtlas*> ResourceManager::Fonts;

Shader ResourceManager::LoadShader(const char* vShaderFile, const char* fShaderFile, const char* gShaderFile, ShaderID name)
{

    Shader shader(vShaderFile, fShaderFile, gShaderFile);
    Shaders[name] = shader;
    return Shaders[name] = shader;
}

Shader& ResourceManager::GetShader(const ShaderID& name)
{
    return Shaders[name];
}

FontAtlas* ResourceManager::LoadFont(const std::string& name, const std::string& fontPath, unsigned int fontSize)
{
    // уже загружен
    if (Fonts.count(name))
        return Fonts[name];

    FontAtlas* font = new FontAtlas(fontPath, fontSize);
    Fonts[name] = font;
    return font;
}

FontAtlas* ResourceManager::GetFont(const std::string& name)
{
    auto it = Fonts.find(name);
    if (it != Fonts.end())
        return it->second;

    return nullptr;
}

Texture2D ResourceManager::LoadTexture(const char* file, bool alpha, std::string name)
{
    Textures[name] = loadTextureFromFIle(file, alpha);
    return Textures[name];
}

Texture2D ResourceManager::LoadPixelTexture(const char* file, bool alpha, std::string name)
{
    Textures[name] = loadPixelTextureFromFile(file, alpha);
    return Textures[name];
}

Texture2D& ResourceManager::GetTexture(const std::string& name)
{
    return Textures[name];
}

void ResourceManager::clear()
{
    for (auto& [_, shader] : Shaders)
    {
        if (shader.ID != 0)
            glDeleteProgram(shader.ID);
    }

    for (auto& [_, texture] : Textures)
    {
        if (texture.ID != 0)
            glDeleteTextures(1, &texture.ID);
    }
    for (auto& [_, font] : Fonts)
    {
        delete font;
    }
    Fonts.clear();
    Shaders.clear();
    Textures.clear();
}

Texture2D ResourceManager::loadTextureFromFIle(const char* File, bool alpha)
{
    //create texture object
    Texture2D texture;
    if (alpha)
    {
        texture.Internal_Format = GL_RGBA;
        texture.Image_Format = GL_RGBA;
    }
    //load image
    int levelWidth, levelHeight, nrChannels;
    unsigned char* data = stbi_load(File, &levelWidth, &levelHeight, &nrChannels, 0);
    //now generate textures
    texture.Generate(data, levelWidth, levelHeight);
    //and finaly free image data
    stbi_image_free(data);
    return texture;
}

Texture2D ResourceManager::loadPixelTextureFromFile(const char* File, bool alpha)
{
    Texture2D texture;
    if (alpha)
    {
        texture.Internal_Format = GL_RGBA;
        texture.Image_Format = GL_RGBA;
    }

    // Жесткие настройки только для этого метода
    texture.Filter_Min = GL_NEAREST;
    texture.Filter_Max = GL_NEAREST;
    texture.Wrap_S = GL_CLAMP_TO_EDGE;
    texture.Wrap_T = GL_CLAMP_TO_EDGE;

    int levelWidth, levelHeight, nrChannels;
    unsigned char* data = stbi_load(File, &levelWidth, &levelHeight, &nrChannels, 0);

    texture.Generate(data, levelWidth, levelHeight);

    stbi_image_free(data);
    return texture;
}
