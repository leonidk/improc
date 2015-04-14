#include "image_io.h"

int main(int argc, char * argv[])
{
	do {
		auto jpg = img::imread("test.jpg");
		auto png = img::imread("test.png");

		img::imshow("jpg", jpg);
		img::imshow("png", png);

	} while( 'q' != img::getKey(false));

	return 0;
}