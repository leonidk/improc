#include "image.h"
#include <string>
#include <algorithm>
#pragma warning(disable : 4996)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace img {
	std::string getExtension(const std::string & str) {
		auto found = str.find_last_of(".");
		auto ext = (found == std::string::npos) ? "" : str.substr(found);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		return ext;
	}
	Image imread(const char * name) {
		Image returnImage = {};
		returnImage.data = std::shared_ptr<void>(stbi_load(name, &returnImage.width, &returnImage.height, &returnImage.channels, 0));
		return returnImage;
	}

	void imwrite(const char * name, const Image &img) {
		auto ext = getExtension(name);

		if (ext == ".png") {
			stbi_write_png(name, img.width, img.height, img.channels, img.data.get(), 0);
		} else if (ext == ".tga") {
			stbi_write_tga(name, img.width, img.height, img.channels, img.data.get());
		} else if (ext == ".bmp") {
			stbi_write_bmp(name, img.width, img.height, img.channels, img.data.get());
		} else if (ext == ".hdr") {
			stbi_write_hdr(name, img.width, img.height, img.channels, (float*)img.data.get());
		}
	}
}