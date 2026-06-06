#ifndef TEXTURE_H
#define TEXTURE_H
#include <glad/glad.h>


class Texture2D {
public:
    // holds the ID of the texture object, used for all texture operations to reference to this particular texture
    unsigned int ID;
    // texture image dimensions
    unsigned int Width, Height; // width and height of loaded image in pixels
    // texture Format
    unsigned int Internal_Format; // format of texture object
    unsigned int Image_Format; // format of loaded image
    // texture configuration
    unsigned int Wrap_S; // wrapping mode on S axis
    unsigned int Wrap_T; // wrapping mode on T axis
    unsigned int Filter_Min; // filtering mode if texture pixels < screen pixels
    unsigned int Filter_Max; // filtering mode if texture pixels > screen pixels

	Texture2D();

	//generate texture
	void Generate(unsigned char* texturePath, unsigned int levelHeight, unsigned int levelWidth);

	//binds texture as the current active GL_Texture_2D tex object
	void Bind() const;


};


#endif // !TEXTURE_H
