#pragma once
#include <glad/glad.h>
#include <iostream>
#include <memory>
#include <stb_image.h>
#include <string>
#include <map>

namespace LEapsGL {
	struct ImageFormat {
		GLenum colorFormat;
		GLenum colorType;
	};
	ImageFormat GetImageFormatFromPath(const std::string path);

	struct Image {
		int width, height, nrChannels;
		ImageFormat format;
		std::shared_ptr<void> pixels;
		int totalByteSize;

		static Image LoadImage(const char*);
		template<typename T> static Image CreateImage(int width, int height, int nrchannel,
						   ImageFormat fmt);
	};
	template<typename T>
	inline Image Image::CreateImage(int width, int height, int nrchannel, ImageFormat fmt)
	{
		Image img = Image();
		img.pixels = std::shared_ptr<T>(new T[width * height * nrchannel]);

		img.width = width;
		img.height = height;
		img.nrChannels = nrchannel;
		img.format = fmt;
		img.totalByteSize = width * height * nrchannel * sizeof(T);

		memset(img.pixels.get(), 0, img.totalByteSize);

		return img;
	}
}