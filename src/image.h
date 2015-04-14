#pragma once
#include <memory>
namespace img {
	enum {
		IM_8U,
		IM_8I,
		IM_16U,
		IM_16I,
		IM_32U,
		IM_32I,
		IM_32F,
		IM_64F
	};

	struct Image {
		std::shared_ptr<void> data;
		int type;
		int width, height, channels;
	};

	///useful for non-owning calls:
	//		img::imshow("tmp", { { tmp, img::null_d }, img::IM_8U, 320, 240, 1 });
	static auto null_d = [](const void * p){};
}