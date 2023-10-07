#include "Texture2D.h"
#include "stb_image.h"
#include <iostream>
#include <filesystem>

LEapsGL::Texture2D::Texture2D(const Image img)
{
	this->img = img;
	this->format = img.format.colorFormat;
}

void LEapsGL::Texture2D::Apply()
{
	bind();
	for (auto iter : textureParams) {
		glTexParameteri(GL_TEXTURE_2D, iter.first, iter.second);
	}

	// state name, mipmap level, gl texteure format, width, height, 0, image format, image data byte
	glTexImage2D(GL_TEXTURE_2D, 0, format, img.width, img.height, 0, img.format.colorFormat, img.format.colorType, img.pixels.get());
	glGenerateMipmap(GL_TEXTURE_2D);
}

void LEapsGL::Texture2D::AllocateDefaultSetting()
{
	glGenTextures(1, &ID);

	// set the texture wrapping parameters
	SetTextureParam(GL_TEXTURE_WRAP_S, GL_REPEAT);
	SetTextureParam(GL_TEXTURE_WRAP_T, GL_REPEAT);

	// set texture filtering parameters
	SetTextureParam(GL_TEXTURE_MIN_FILTER, GL_REPEAT);
	SetTextureParam(GL_TEXTURE_MAG_FILTER, GL_REPEAT);
}

void LEapsGL::Texture2D::bind()
{
	glBindTexture(GL_TEXTURE_2D, ID);
}

void LEapsGL::Texture2D::SetTextureParam(GLuint key, GLuint value)
{
	textureParams[key] = value;
}

GLuint LEapsGL::Texture2D::getID()
{
	return ID;
}

void LEapsGL::Texture2D::setID(GLuint id)
{
	ID = id;
}

LEapsGL::Texture2D LEapsGL::InitSimpleTexture(Color c)
{
	auto img = LEapsGL::Image::CreateImage<GLubyte>(1, 1, 3, LEapsGL::ImageFormat{ GL_RGB, GL_UNSIGNED_BYTE });
	GLubyte* img_raws = static_cast<GLubyte*>(img.pixels.get());
	img_raws[0] = GLubyte(c.r * 255);
	img_raws[1] = GLubyte(c.g * 255);
	img_raws[2] = GLubyte(c.b * 255);
	return LEapsGL::Texture2D(img);;
}

const LEapsGL::Texture2D LEapsGL::Texture2D::blackTexture = LEapsGL::InitSimpleTexture(LEapsGL::Color::black);
const LEapsGL::Texture2D LEapsGL::Texture2D::grayTexture = LEapsGL::InitSimpleTexture(LEapsGL::Color::gray);
