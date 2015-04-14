#include "image_io.h"
#include "image_filter.h"
int main(int argc, char * argv[])
{
	do {
		auto jpg = img::imread("test.jpg");
		auto png = img::imread("test.png");
		auto jpg2 = img::boxFilter<11>(jpg);
		//img::imshow("jpg", jpg);
		img::imshow("jpg2", jpg2);
		//img::imshow("png", png);

	} while( 'q' != img::getKey(false));

	return 0;
}