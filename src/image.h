#pragma once
#include <memory>
namespace img {
	
	template <typename T, int C>
	struct Image {
		std::shared_ptr<T> data;
		int width, height;
	};
	///useful for non-owning calls:
	//		img::imshow("tmp", { { tmp, img::null_d }, img::IM_8U, 320, 240, 1 });
	static auto null_d = [](const void * p){};

	//visual studio doesn't seem to care about needing this to allocate arrays
	//although it should probably be needed?
	template< typename T >
	struct arr_d {void operator ()(T const * p)	{ delete[] p; } };

}