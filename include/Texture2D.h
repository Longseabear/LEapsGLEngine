#pragma once

/*
    GL dependency Texture2D class.
*/

#include <glad/glad.h>
#include "Object.h"
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>

#include <core/Proxy.h>
#include <core/Type.h>
#include <Color.h>
#include <Image.h>

#include "stb_image.h"
using namespace std;

using TextureFormat = GLenum;
namespace LEapsGL {

    enum class TextureType {
        IMAGE, DIFFUSE, NORMAL, UV, SPECULAR, HEIGHT, COUNT
    };

    inline std::string getTextureTypeName(TextureType t) {
        switch (t)
        {
        case TextureType::IMAGE:
            return "image";
        case TextureType::DIFFUSE:
            return "diffuse";
        case TextureType::HEIGHT:
            return "height";
        case TextureType::NORMAL:
            return "normal";
        case TextureType::UV:
            return "uv";
        case TextureType::SPECULAR:
            return "specular";
        case TextureType::COUNT:
            return "unknown";
        }
    }

    struct TextureEnumCounter {
        TextureEnumCounter() {
            Counter = vector<int>(size_t(TextureType::COUNT), 0);
        }
        int getCount(TextureType t) {
            return Counter[int(t)]++;
        }
        vector<int> Counter;
    };


	class Texture2D : Object {
	public:
		Texture2D() :ID(0), format(GL_RGB), mipmapCount(0), img(){};
		Texture2D(const Image img);
		Texture2D(const Texture2D& rhs): Object(){
			this->ID = rhs.ID;
			format = rhs.format;
			mipmapCount = mipmapCount;
			img = rhs.img;
            type = rhs.type;

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
        // don't need move operator in copy and swap idiom.
		//Texture2D& operator=(Texture2D&& other) noexcept {
		//	swap(*this, other);
		//	return *this;
		//}
		friend void swap(Texture2D& lhs, Texture2D& rhs) {
			using std::swap;
			swap(lhs.format, rhs.format);
			swap(lhs.ID, rhs.ID);
			swap(lhs.mipmapCount, rhs.mipmapCount);
			swap(lhs.textureParams, rhs.textureParams);
			swap(lhs.img, rhs.img);
            swap(lhs.type, rhs.type);
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

        Image& getImage() {
            return img;
        }

        void setType(TextureType t) {
            this->type = t;
        }
        TextureType getType() {
            return this->type;
        }
	private:
		GLuint ID;
		GLuint mipmapCount;

		Image img;
		TextureFormat format;
        TextureType type;
		std::map<GLuint, GLuint> textureParams;
	};

	Texture2D InitSimpleTexture(Color);

    struct TextureGroup {};

    struct TextureSpecification : public LEapsGL::ProxyRequestSpecification<Texture2D> {
        TextureType type;
    };
    struct TextureFromFileSpecification : public TextureSpecification {
    public:
        using Self = TextureFromFileSpecification;

        // Required::
        // ---------------------------------------------
        using instance_type = Texture2D; // Type of object to create
        using proxy_group = TextureGroup; // If a group is specified, it is stored in the same world.
        // ---------------------------------------------

        LEapsGL::PathString path;

        virtual instance_type generateInstance() const {
            // Implement an object creation method for a given specification
            auto tex = Texture2D(LEapsGL::Image::LoadImage(path.c_str()));
            tex.setType(type);
            tex.AllocateDefaultSetting();
            tex.Apply();
            return tex;
        }
        virtual size_t hash() override
        {            
            // Implement an object creation method for a given specification
            return LEapsGL::PathString::FixedStringHashFn{}(path);
        }
    };
    struct TextureFromBlankImageSpecification : public TextureSpecification {
    public:
        using Self = TextureFromBlankImageSpecification;

        // Required::
        // ---------------------------------------------
        using instance_type = Texture2D; // Type of object to create
        using proxy_group = TextureGroup; // If a group is specified, it is stored in the same world.
        // ---------------------------------------------

        int width, height, nrchannel;
        LEapsGL::ImageFormat fmt;

        // -----------------------------------
        virtual instance_type generateInstance() const {
            auto tex = LEapsGL::Texture2D(LEapsGL::Image::CreateImage<GLubyte>(width, height, nrchannel, LEapsGL::ImageFormat{ GL_RGB, GL_UNSIGNED_BYTE }));
            tex.setType(type);
            tex.AllocateDefaultSetting();
            tex.Apply();
            return tex;
        }
        virtual size_t hash() override
        {
            size_t h = LEapsGL::HASH_RANDOM_SEED;
            LEapsGL::hash_combine(h, width, height, nrchannel, fmt.colorFormat, fmt.colorType);
            // Implement an object creation method for a given specification
            return h;
        }
    };

    struct Texture2DFactory {
        using RequestorType = LEapsGL::ProxyRequestor<LEapsGL::ProxyEntity<TextureGroup>, Texture2D>;

        static auto from_file(const PathString path, TextureType type = TextureType::IMAGE) {
            TextureFromFileSpecification instance;
            instance.path = path;
            instance.type = type;
            return LEapsGL::ProxyTraits::Get<TextureFromFileSpecification>(instance);
        }
        static auto from_blank(int width, int height, int nrchannel, LEapsGL::ImageFormat fmt, TextureType type = TextureType::IMAGE) {
            TextureFromBlankImageSpecification instance;
            instance.width = width;
            instance.height = height;
            instance.nrchannel = nrchannel;
            instance.fmt = fmt;
            instance.type = type;

            return LEapsGL::ProxyTraits::Get<TextureFromBlankImageSpecification>(instance);
        }
    };
    
};

