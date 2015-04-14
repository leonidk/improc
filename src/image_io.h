#pragma once
#include "image.h"

namespace img {
	//imio.cpp
	Image imread(const char * name);
	void imwrite(const char * name, const Image &img);

	//imshow.cpp
	void imshow(const char * name, const Image &img);
	char getKey(bool wait = false);
}