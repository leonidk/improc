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

	template <typename T>
	inline int getIMType();

	template <>
	inline int getIMType<uint8_t>() { return IM_8U; }
	template <>
	inline int getIMType<int8_t>() { return IM_8I; }
	template <>
	inline int getIMType<uint16_t>() { return IM_16U; }
	template <>
	inline int getIMType<int16_t>() { return IM_16I; }
	template <>
	inline int getIMType<uint32_t>() { return IM_32U; }
	template <>
	inline int getIMType<int32_t>() { return IM_32I; }
	template <>
	inline int getIMType<float>() { return IM_32F; }
	template <>
	inline int getIMType<double>() { return IM_64F; }

}