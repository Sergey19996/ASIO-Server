#include "Texture.h"
#include <iostream>
Texture::Texture() : Width(0), Height(0), Internal_Format(GL_RGB), Image_Format(GL_RGB), Wrap_S(GL_REPEAT), Wrap_T(GL_REPEAT), Filter_Max(GL_LINEAR), Filter_Min(GL_LINEAR_MIPMAP_LINEAR)
{
	glGenTextures(1, &this->ID);
}

void Texture::Generate(unsigned char* texturePath, unsigned int height, unsigned int width)
{


	this->Width = width;
	this->Height = height;
	//create Tex
	glBindTexture(GL_TEXTURE_2D, ID);
	glTexImage2D(GL_TEXTURE_2D, 0, this->Internal_Format, width, height, 0, this->Image_Format, GL_UNSIGNED_BYTE, texturePath);  //создаёт текстуру по ширине и высоте и записывает индекс
	glGenerateMipmap(GL_TEXTURE_2D); // Создаёт разного разрешения семплы текстуры и берёт с них цвета при изменения размера  (используем только для уменьшения)
	//set Texture wrap as filtering modes
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, Wrap_S); //GL_REPEAT by default
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, Wrap_T);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Filter_Min);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Filter_Max);

	glBindTexture(GL_TEXTURE_2D, 0);

}

void Texture::Bind() const
{
	glBindTexture(GL_TEXTURE_2D, this->ID);
}
