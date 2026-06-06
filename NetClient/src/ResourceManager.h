#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H
#include <map>
#include <glad/glad.h>
#include "rendering/Texture.h"
#include "Rendering/Shader.h"
#include "Text/FontAtlas.h"

enum class ShaderID {
	Sprite,
	UIGlyph,
	CooldownRadial,
	TextShader,
	FogMask,
	FogBlur,
	LightShadow,
	FogCopy,
	World,
	instanceWorld,
	Minimap
};


class ResourceManager
{
public:
	static std::map<ShaderID, Shader> Shaders;
	static std::map<std::string, Texture2D>Textures;
	static std::unordered_map<std::string, FontAtlas*> Fonts;


	static Shader LoadShader(const char* vShaderFile, const char* fShaderFile, const char* gShaderFile, ShaderID name);
	static Shader& GetShader(const ShaderID&name);

	static FontAtlas* LoadFont(const std::string& name, const std::string& fontPath, unsigned int fontSize);
	static FontAtlas* GetFont(const std::string& name);

	static Texture2D LoadTexture(const char* file, bool alpha, std::string name);
	static Texture2D LoadPixelTexture(const char* file, bool alpha, std::string name);
	static Texture2D& GetTexture(const std::string& name);
	//properly de-allocates all loaded resources
	static void clear();


private:
	ResourceManager() {}


	static Texture2D loadTextureFromFIle(const char* File, bool alpha);
	static Texture2D loadPixelTextureFromFile(const char* File, bool alpha);
};


#endif // !RESOURCEMANAGER_H
