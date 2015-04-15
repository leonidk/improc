#pragma once
#include "image.h"

namespace img {
	namespace detail {
		template <typename T, typename TT>
		Image _grey2Rgb(const Image & input){
			Image output = { std::shared_ptr<void>(new T[input.width*input.height * 3]), input.type, input.width, input.height, 3 };
			T* in_data = (T*)input.data.get();
			T* ot_data = (T*)output.data.get();
			auto stride = input.width*3;
			for (int y = 0; y < input.height; y++) {
				for (int x = 0; x < input.width; x++) {
					for (int c = 0; c < 3; c++){
						ot_data[y*stride + x * 3 + c] = in_data[y*input.width + x];
					}
				}
			}
			return output;
		}
		template <typename T, typename TT>
		Image _Rgb2grey(const Image & input){
			Image output = { std::shared_ptr<void>(new T[input.width*input.height]), input.type, input.width, input.height, 1 };
			T* in_data = (T*)input.data.get();
			T* ot_data = (T*)output.data.get();
			auto stride = input.width * 3;
			for (int y = 0; y < input.height; y++) {
				for (int x = 0; x < input.width; x++) {
					TT sum = 0;
					for (int c = 0; c < 3; c++){
						sum += in_data[y*stride + x * 3 + c];
					}
					ot_data[y*input.width + x] = (sum+2)/3;

				}
			}
			return output;
		}
		template <typename T, typename TT>
		Image _intImage_1C(const Image & input,int outType){
			Image output = { std::shared_ptr<void>(new TT[input.width*input.height*input.channels]), outType, input.width, input.height, input.channels };
			T* in_data = (T*)input.data.get();
			TT* ot_data = (TT*)output.data.get();
			for (int x = 0; x < input.width; x++) {
				ot_data[x] = in_data[x]
			}
			for (int y = 1 y < input.height; y++) {
				TT sum = 0;
				for (int x = 0; x < input.width ; x++) {
					sum += in_data[y*input.width + x];
					ot_data = sum + in_data[(y-1)*input.width + x];
				}
			}
			return output;
		}
		template <typename T, typename TT, int k_w>
		Image _boxFilter(const Image & input){
			Image output = { std::shared_ptr<void>(new T[input.width*input.height*input.channels]), input.type, input.width, input.height, input.channels };
			T* in_data = (T*)input.data.get();
			T* ot_data = (T*)output.data.get();
			memcpy(ot_data, in_data, sizeof(T)*input.width*input.height*input.channels);
			auto stride = input.width*input.channels;
			auto ch = input.channels;
			auto oset = k_w / 2;
			for (int y = oset; y < input.height - oset; y++) {
				for (int x = oset; x < input.width - oset; x++) {
					for (int c = 0; c < input.channels; c++) {
						TT tmp = 0;
						for (int o = -oset; o <= oset; o++) {
							tmp += in_data[y * stride + (x + o)*ch + c];
						}
						ot_data[y * stride + x*ch + c] = (tmp + k_w - 1) / k_w;
					}
				}
			}
			for (int y = oset; y < input.height - oset; y++) {
				for (int x = oset; x < input.width - oset; x++) {
					for (int c = 0; c < input.channels; c++) {
						TT tmp = 0;
						for (int o = -oset; o <= oset; o++) {
							tmp += ot_data[(y + o) * stride + x*ch + c];
						}
						ot_data[y * stride + x*ch + c] = (tmp + k_w - 1) / k_w;
					}
				}
			}
			for (int y = input.height - oset; y < input.height; y++) {
				for (int x = 0; x < input.width; x++) {
					for (int c = 0; c < input.channels; c++) {
						ot_data[y * stride + x*ch + c] = in_data[y * stride + x*ch + c];
					}
				}
			}
			return output;
		}
	}
	template <int k_w>
	Image boxFilter(const Image & input){
		switch (input.type) {
		case IM_8U:
			return detail::_boxFilter<uint8_t,uint16_t,k_w>(input);
		case IM_8I:
			return detail::_boxFilter<int8_t,int16_t, k_w>(input);
		case IM_16U:
			return detail::_boxFilter<uint16_t,uint32_t, k_w>(input);
		case IM_16I:
			return detail::_boxFilter<int16_t, int32_t,k_w>(input);
		case IM_32U:
			return detail::_boxFilter<uint32_t, uint32_t, k_w>(input);
		case IM_32I:
			return detail::_boxFilter<int32_t, int32_t,k_w>(input);
		case IM_32F:
			return detail::_boxFilter<float, float, k_w>(input);
		case IM_64F:
			return detail::_boxFilter<double, double, k_w>(input);
		default:
			return{};
		}
	}
	Image grey2Rgb(const Image & input){
		switch (input.type) {
		case IM_8U:
			return detail::_grey2Rgb<uint8_t, uint16_t>(input);
		case IM_8I:
			return detail::_grey2Rgb<int8_t, int16_t>(input);
		case IM_16U:
			return detail::_grey2Rgb<uint16_t, uint32_t>(input);
		case IM_16I:
			return detail::_grey2Rgb<int16_t, int32_t>(input);
		case IM_32U:
			return detail::_grey2Rgb<uint32_t, uint32_t>(input);
		case IM_32I:
			return detail::_grey2Rgb<int32_t, int32_t>(input);
		case IM_32F:
			return detail::_grey2Rgb<float, float>(input);
		case IM_64F:
			return detail::_grey2Rgb<double, double>(input);
		default:
			return{};
		}
	}
	Image Rgb2grey(const Image & input){
		switch (input.type) {
		case IM_8U:
			return detail::_Rgb2grey<uint8_t, uint16_t>(input);
		case IM_8I:
			return detail::_Rgb2grey<int8_t, int16_t>(input);
		case IM_16U:
			return detail::_Rgb2grey<uint16_t, uint32_t>(input);
		case IM_16I:
			return detail::_Rgb2grey<int16_t, int32_t>(input);
		case IM_32U:
			return detail::_Rgb2grey<uint32_t, uint32_t>(input);
		case IM_32I:
			return detail::_Rgb2grey<int32_t, int32_t>(input);
		case IM_32F:
			return detail::_Rgb2grey<float, float>(input);
		case IM_64F:
			return detail::_Rgb2grey<double, double>(input);
		default:
			return{};
		}
	}
}