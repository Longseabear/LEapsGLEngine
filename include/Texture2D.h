#pragma once


#include <glad/glad.h>
#include "Object.h"
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>

#include <Color.h>
#include <Image.h>
#include "stb_image.h"
using namespace std;

using TextureFormat = GLenum;

namespace LEapsGL {
	class Texture2D : Object {
	public:
		Texture2D() :ID(0), format(GL_RGB), mipmapCount(0), img(){};
		Texture2D(const Image img);
		Texture2D(const Texture2D& rhs): Object(){
			this->ID = rhs.ID;
			format = rhs.format;
			mipmapCount = mipmapCount;
			img = rhs.img;

			int size = rhs.img.totalByteSize;
			textureParams = rhs.textureParams;
		}
		Texture2D(Texture2D&& rhs) noexcept : Texture2D(){
			swap(*this, rhs);
		}
		// Move constructor
		Texture2D& operator=(Texture2D other) {
			swap(*this, other);
			return *this;
		}
		Texture2D& operator=(Texture2D&& other) noexcept {
			swap(*this, other);
			return *this;
		}
		friend void swap(Texture2D& lhs, Texture2D& rhs) {
			using std::swap;
			swap(lhs.format, rhs.format);
			swap(lhs.ID, rhs.ID);
			swap(lhs.mipmapCount, rhs.mipmapCount);
			swap(lhs.textureParams, rhs.textureParams);
			swap(lhs.img, rhs.img);
		}

		// upload current image texture buffer CPU to GPU
		void Apply();
		void AllocateDefaultSetting();

		//	void MakeEmptyTexture(int width, int height, int nrChannel);

		void bind();
		void SetTextureParam(GLuint key, GLuint value);
		GLuint getID();
		void setID(GLuint id);

		static const Texture2D blackTexture;
		static const Texture2D grayTexture;

	private:
		GLuint ID;
		GLuint mipmapCount;

		Image img;		
		TextureFormat format;
		std::map<GLuint, GLuint> textureParams;

	};

	Texture2D InitSimpleTexture(Color);
};

