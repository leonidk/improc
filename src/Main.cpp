#include "image.h"

int main(int argc, char * argv[])
{
	auto jpg = img::imread("test.jpg");
	auto png = img::imread("test.jpg");
	img::imshow("jpg", jpg);
	img::imshow("png", png);
	img::getKey(true);
	return 0;
}