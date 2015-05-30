#include "image_io.h"
#include "image_filter.h"

#ifdef _WIN32 
#include <intrin.h>
#define bSwap _byteswap_ulong
#else
#define bSwap __builtin_bswap32
#endif
#include <vector>
#include <fstream>
std::vector<img::Img<uint8_t>> readMNISTImg(const std::string &fileName)
{
	std::vector<img::Img<uint8_t>> imgs;
	std::ifstream file(fileName,std::ios::binary);
	if (file.is_open())
	{
		uint32_t magic_number = 0;
		uint32_t number_of_images = 0;
		uint32_t n_rows = 0;
		uint32_t n_cols = 0;
		file.read((char*)&magic_number, sizeof(magic_number));
		magic_number = bSwap(magic_number);
		file.read((char*)&number_of_images, sizeof(number_of_images));
		number_of_images = bSwap(number_of_images);
		file.read((char*)&n_rows, sizeof(n_rows));
		n_rows = bSwap(n_rows);
		file.read((char*)&n_cols, sizeof(n_cols));
		n_cols = bSwap(n_cols);
		for (int i = 0; i<number_of_images; ++i)
		{
			img::Img<uint8_t> img(n_cols,n_rows);
			file.read((char*)img.data.get(), n_rows*n_cols);
			imgs.push_back(img);
		}
	}

	return imgs;
}

std::vector<uint8_t> readMNISTLabel(const std::string &fileName)
{
	std::vector<uint8_t> labels;
	std::ifstream file(fileName, std::ios::binary);
	if (file.is_open())
	{
		uint32_t magic_number = 0;
		uint32_t n_items = 0;
		file.read((char*)&magic_number, sizeof(magic_number));
		magic_number = bSwap(magic_number);
		file.read((char*)&n_items, sizeof(n_items));
		n_items = bSwap(n_items);
		for (int i = 0; i<n_items; ++i)
		{
			uint8_t label;
			file.read((char*)&label, 1);
			labels.push_back(label);
		}
	}
	return labels;
}


int main(int argc, char * argv[])
{
	auto train_img = readMNISTImg("train-images.idx3-ubyte");
	auto train_lbl = readMNISTLabel("train-labels.idx1-ubyte");
	int idx = 0;
	do {
		auto tl = train_lbl[idx];
		auto t1 = train_img[idx++];
		img::imshow("label",t1);
		auto jpg = img::imread<uint8_t,3>("test.jpg");
		auto png = img::imread<uint8_t, 3>("test.png");
		auto jpg2 = img::boxFilter<11>(jpg);
		img::imshow("jpg", jpg);
		img::imshow("jpg2", jpg2);

		auto gjpg = img::Rgb2grey(jpg);
		jpg2 = img::grey2Rgb(gjpg);
		auto gjpg2 = img::boxFilter<11>(gjpg);
		img::imshow("jpg23", gjpg2);

		auto intImg = img::intImage<uint8_t,1,uint32_t>(gjpg);
		img::imshow("gjpg", gjpg);
		img::imshow("png", png);
	} while( 'q' != img::getKey(false));

	return 0;
}