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


int main(int argc, char * argv[])
{

    auto png = img::imread<uint8_t, 3>("test.png");
    do {
        auto gjpg = img::Rgb2grey(png);
        auto dt = img::domainTransform(gjpg, gjpg, 3,40, 12.0f);
        img::imshow("grey", gjpg);
        img::imshow("dt", dt);
        auto dt2 = img::domainTransform(png, png, 3, 40, 3*12.0f);
        img::imshow("rgb", png);
        img::imshow("dt-rgb",dt2 );

    } while ( img::getKey() != 'q' );

    return 0;
}