#include "image.h"

#pragma warning(disable : 4996)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace img {
	Image imread(const char * name) {
		return{};
	}

	void imwrite(const char * name, const Image &img) {

	}
}