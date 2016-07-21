
#include "../src/image.h"
#include "../src/image_io.h"
#include "../src/image_filter.h"

#include <vector>
#include <fstream>
#include <chrono>
#include <iostream>

#include <Halide.h>
#include <Windows.h>
struct HighResClock
{
    typedef long long                               rep;
    typedef std::nano                               period;
    typedef std::chrono::duration<rep, period>      duration;
    typedef std::chrono::time_point<HighResClock>   time_point;
    static const bool is_steady = true;

    static time_point now();
};
namespace
{
    const long long g_Frequency = []() -> long long
    {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        return frequency.QuadPart;
    }();
}

HighResClock::time_point HighResClock::now()
{
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return time_point(duration(count.QuadPart * static_cast<rep>(period::den) / g_Frequency));
}
#define AOT (0)
#if AOT
#define GENERATE (0)
#if GENERATE
Halide::Func blur_3x3(Halide::ImageParam & himg) {
    Halide::Func input, output, clamped_input;
    Halide::Func blur_x, blur_y;
    Halide::Var x, y, xi, yi;
    input(x, y) = Halide::cast<uint16_t>(himg(x, y));
    clamped_input = Halide::BoundaryConditions::repeat_edge(input);

    // The algorithm - no storage or order
    blur_x(x, y) = (clamped_input(x - 1, y) + clamped_input(x, y) + clamped_input(x + 1, y)) / 3;
    blur_y(x, y) = (blur_x(x, y - 1) + blur_x(x, y) + blur_x(x, y + 1)) / 3;

    // The schedule - defines order, locality; implies storage
    blur_y.tile(x, y, xi, yi, 256, 32)
        .vectorize(xi, 8).parallel(y);
    blur_x.compute_at(blur_y, x).vectorize(x, 8);
    blur_y.compute_root();

    output(x, y) = Halide::cast<uint8_t>(blur_y(x, y));


    return output;
}
int main(int argc, char * argv[])
{
    Halide::ImageParam himg(Halide::type_of<uint8_t>(), 2);
    std::vector<Halide::Argument> args = { himg };
    blur_3x3(himg).compile_to_file(std::string("blur"), args);
    return 0;
}
#else
#include "blur.h"
#pragma comment(lib, "blur.lib")

int main(int argc, char * argv[])
{

    auto png = img::imread<uint8_t, 3>("test.png");
    int i = 1;

    img::Image<uint8_t, 1> myh(png.width, png.height);

    auto gjpg = img::Rgb2grey(png);
    buffer_t input_buf = { 0 }, output_buf = { 0 };
    input_buf.host = gjpg.ptr;
    output_buf.host = myh.ptr;
    input_buf.stride[0] = output_buf.stride[0] = 1;
    // stride[1] is the width of the image, because pixels that are
    // adjacent in y are separated by a scanline's worth of pixels in
    // memory.
    input_buf.stride[1] = gjpg.width;
    output_buf.stride[1] = myh.width-2;
    // The extent tells us how large the image is in each dimension.
    input_buf.extent[0] = gjpg.width;
    input_buf.extent[1] = gjpg.height;
    output_buf.extent[0] = myh.width - 2;
    output_buf.extent[1] = myh.height-2;
    input_buf.elem_size = output_buf.elem_size = 1;
    output_buf.min[0] = 1;
    output_buf.min[1] = 1;
    do {

        auto start = HighResClock::now();
        blur(&input_buf, &output_buf);
        auto end = HighResClock::now();
        auto diff = end - start;
        std::cout << std::chrono::duration_cast<std::chrono::microseconds>(diff).count() << " us" << std::endl;

        //auto dt = img::domainTransform(gjpg, gjpg, 3, 40, i % 128);
        //img::imshow("grey", gjpg);
        //img::imshow("dt", dt);
        //img::imshow("halide", myh);

        //auto dt2 = img::domainTransform(png, png,3,40, 3 * (i % 128));
        //img::imshow("rgb", png);
        //img::imshow("dt-rgb", dt2);
        //printf("\r%d", i);
        //i += 5;
    } while (img::getKey() != 'q');

    return 0;
}
#endif
#else
#include "../src/image.h"
#include "../src/image_io.h"
#include "../src/image_filter.h"

#include <vector>
#include <fstream>
#include <chrono>
#include <iostream>

#include <Halide.h>

Halide::Func blur_3x3(Halide::Image<uint8_t> & himg) {
    Halide::Func input, output;
    Halide::Func blur_x, blur_y;
    Halide::Var x, y, xi, yi;
    input(x, y) = Halide::cast<uint16_t>(himg(x, y));

    // The algorithm - no storage or order
    blur_x(x, y) = (input(x - 1, y) + input(x, y) + input(x + 1, y)) / 3;
    blur_y(x, y) = (blur_x(x, y - 1) + blur_x(x, y) + blur_x(x, y + 1)) / 3;

    // The schedule - defines order, locality; implies storage
    blur_y.tile(x, y, xi, yi, 256, 32)
        .vectorize(xi, 8).parallel(y);
    blur_x.compute_at(blur_y, x).vectorize(x, 8);
    blur_y.compute_root();

    output(x, y) = Halide::cast<uint8_t>(blur_y(x, y));


    return output;
}
int main(int argc, char * argv[])
{

    auto png = img::imread<uint8_t, 3>("test.png");
    int i = 1;

    img::Image<uint8_t, 1> myh(png.width - 2, png.height - 2);

    Halide::Image<uint8_t> himg(png.width, png.height);
    Halide::Image<uint8_t> hout(png.width - 2, png.height - 2);
    auto gjpg = img::Rgb2grey(png);
    auto s0 = himg.stride(0);
    auto s1 = himg.stride(1);
    memcpy(himg.data(), gjpg.ptr, png.width*png.height);
    hout.set_min(1, 1);
    auto hfunc = blur_3x3(himg);
    hfunc.compile_jit();
    do {
        auto start = HighResClock::now();
        hfunc.realize(hout);
        auto end = HighResClock::now();
        auto diff = end - start;
        std::cout << std::chrono::duration_cast<std::chrono::microseconds>(diff).count() << " us" << std::endl;
        memcpy(myh.ptr, hout.data(), myh.width*myh.height);

        //auto dt = img::domainTransform(gjpg, gjpg, 3, 40, i % 128);
        //img::imshow("grey", gjpg);
        //img::imshow("dt", dt);
        //img::imshow("halide", myh);

        //auto dt2 = img::domainTransform(png, png,3,40, 3 * (i % 128));
        //img::imshow("rgb", png);
        //img::imshow("dt-rgb", dt2);
        //printf("\r%d", i);
        //i += 5;
    } while (img::getKey() != 'q');

    return 0;
}
#endif