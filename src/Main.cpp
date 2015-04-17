#include "image_io.h"
#include "image_filter.h"
int main(int argc, char * argv[])
{
	do {
		auto jpg = img::imread<uint8_t,3>("test.jpg");
		auto png = img::imread<uint8_t, 3>("test.png");
		auto jpg2 = img::boxFilter<11>(jpg);
		img::imshow("jpg", jpg);
		img::imshow("jpg2", jpg2);

		auto gjpg = img::Rgb2grey(jpg);
		jpg2 = img::grey2Rgb(gjpg);
		auto gjpg2 = img::boxFilter<11>(gjpg);
		img::imshow("jpg23", gjpg2);

		auto intImg = img::intImage_1C<uint8_t,uint32_t>(gjpg);
		img::imshow("gjpg", gjpg);
		img::imshow("png", png);
	} while( 'q' != img::getKey(false));

	return 0;
}