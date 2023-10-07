#include <Image.h>

using namespace LEapsGL;
using namespace std;


ImageFormat LEapsGL::GetImageFormatFromPath(const string path) {
	static std::map<string, ImageFormat> extToFormat = {
		{"jpg", {GL_RGB, GL_UNSIGNED_BYTE}}, {"png", {GL_RGBA, GL_UNSIGNED_BYTE}}
	};


	std::string::size_type idx;
	idx = path.rfind('.');

	if (idx != std::string::npos)
	{
		std::string extension = path.substr(idx + 1);
		auto iter = extToFormat.find(extension);
		if (iter != extToFormat.end()) return iter->second;
	}
	std::cout << "Texture2D:: Format setting fail!!\n";
	return ImageFormat();
}

Image LEapsGL::Image::LoadImage(const char* path)
{
	Image img;
	stbi_set_flip_vertically_on_load(true);
	img.pixels = std::shared_ptr<GLubyte>(stbi_load(path, &img.width, &img.height, &img.nrChannels, 0),
		stbi_image_free);

	if (img.pixels == nullptr) {
		std::cout << "failed to load texture\n";
		return Image();
	}

	img.format = GetImageFormatFromPath(path);
	img.totalByteSize = img.width * img.height * img.nrChannels * sizeof(GLubyte);

	return img;
}
