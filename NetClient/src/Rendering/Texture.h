#ifndef TEXTURE_H
#define TEXTURE_H
#include <glad/glad.h>


class Texture {
public:
	unsigned int ID;

	unsigned int Width, Height;

	unsigned int Internal_Format;
	unsigned int Image_Format;

	//tex configuration
	unsigned int Wrap_S, Wrap_T, Filter_Min, Filter_Max;

	Texture();

	//generate texture
	void Generate(unsigned char* texturePath, unsigned int height, unsigned int width);

	//binds texture as the current active GL_Texture_2D tex object
	void Bind() const;


};


#endif // !TEXTURE_H
