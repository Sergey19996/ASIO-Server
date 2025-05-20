#ifndef  SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <iostream>
#include <glm/glm.hpp>


class Shader
{
public:
	//thie program ID
	unsigned int ID;

	Shader();
	//constructor read and builds the shdaer
	Shader(const char* vertePath, const char* fragmentPath, const char* fgeometryShader = nullptr);
	//use/activate the shader
	Shader& use();
	//utility uniform functions
	void setBool(const std::string& name, bool value) const;
	void setInt(const std::string& name, int value) const;
	void setFloat(const std::string& name, float value) const;
	void setFloatArray(const std::string& name, float* firstvalue, int size) const;
	void setIntArray(const std::string& name, int* array, int size) const;
	void setMat4(const std::string& name, glm::mat4 matrix, bool useShader = false);
	void setivec2(const std::string& name, glm::ivec2 value);
	void setvec2(const std::string& name, glm::vec2 value);
	void setvec4(const std::string& name, glm::vec4 value);
	void setvec3f(const std::string& name, glm::vec3 value);
};


#endif // ! SHADER_H
