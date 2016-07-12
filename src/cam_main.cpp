#include "image_io.h"
#include "image_filter.h"
#include "cam_util.h"
#include <iostream>
#include <array>

#include <librealsense\rs.hpp>

void left_fill(uint16_t in_out[], int w, int h)
{
    for (int y = 0; y < h; y++) {
        int x = 0;
        auto fill = in_out[y*w];
        for (x = 0; x < w && fill == USHRT_MAX; x++) {
            fill = in_out[y*w + x];
        }
        for (x = x - 1; x >= 0; x--) {
            in_out[y*w + x] = fill;
        }
        for (int x = 0; x < w; x++) {
            fill = (in_out[y*w + x] != USHRT_MAX) ? in_out[y*w + x] : fill;
            in_out[y*w + x] = (in_out[y*w + x] != USHRT_MAX) ? in_out[y*w + x] : fill;
        }
    }
}


int main(int argc, char * argv[])
{
    rs::context ctx;
    if (ctx.get_device_count() == 0) throw std::runtime_error("No device detected. Is it plugged in?");
    rs::device & dev = *ctx.get_device(0);
    try {
        dev.enable_stream(rs::stream::depth, 480, 360, rs::format::disparity16, 30);
        dev.enable_stream(rs::stream::color, 640, 480, rs::format::rgb8, 30);

        // Start our device
        dev.start();

        dev.set_option(rs::option::r200_emitter_enabled, true);
        dev.set_option(rs::option::r200_lr_auto_exposure_enabled, true);
        rs_apply_depth_control_preset((rs_device*)&dev, 2); // 2 = low, 4=opt, 5 = high
    }
    catch (std::exception & e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    auto intrin = dev.get_stream_intrinsics(rs::stream::depth);
    auto extrin = dev.get_extrinsics(rs::stream::infrared2, rs::stream::infrared);

    auto fB = intrin.fx * extrin.translation[0] * 1000.0f;
    int counter = 0;

    do {
        dev.wait_for_frames();
        //depth based
        {
            auto w = dev.get_stream_width(rs::stream::depth);
            auto h = dev.get_stream_height(rs::stream::depth);

            auto disp = dev.get_frame_data(rs::stream::depth);

            auto dimg = img::Img<uint16_t>(w, h, (uint16_t*)disp);
            auto dfill = dimg.copy(); left_fill(dfill.ptr, w, h);


            auto ppdisp = img::domainTransformDepth(dimg, dimg, 3, 13, 16);
            auto ppfill = img::domainTransform(dfill, dfill, 2, 13, 16);

            { //output stuff
                auto himg = img::Image<uint8_t, 3>(w, h);
                auto himg2 = img::Image<uint8_t, 3>(w, h);
                auto himg3 = img::Image<uint8_t, 3>(w, h);
                auto himg4 = img::Image<uint8_t, 3>(w, h);
                auto himg5 = img::Image<uint8_t, 3>(w, h);

                util::ConvertDepthToRGBUsingHistogram(himg.ptr, dimg.ptr, w, h, 0.1f, 0.625f);
                util::ConvertDepthToRGBUsingHistogram(himg2.ptr, ppdisp.ptr, w, h, 0.1f, 0.625f);

                util::ConvertDepthToRGBUsingHistogram(himg4.ptr, dfill.ptr, w, h, 0.1f, 0.625f);
                util::ConvertDepthToRGBUsingHistogram(himg5.ptr, ppfill.ptr, w, h, 0.1f, 0.625f);
                auto ppdepth = img::Img<uint16_t>(w, h);
                for (int i = 0; i < w*h; i++) {
                    ppdepth.ptr[i] = fB / (ppfill.ptr[i] / 32.0f);
                }
                util::ConvertDepthToRGBUsingHistogram(himg3.ptr, ppdepth.ptr, w, h, 0.1f, 0.625f);
                img::imshow("original", himg);
                img::imshow("disp-pp", himg2);
                img::imshow("depth-pp", himg3);
                img::imshow("disp-fill", himg4);
                img::imshow("disp-fill-pp", himg5);
            }
        }
        { // color based
            auto w = dev.get_stream_width(rs::stream::rectified_color);
            auto h = dev.get_stream_height(rs::stream::rectified_color);

            auto disp = dev.get_frame_data(rs::stream::depth_aligned_to_rectified_color);
            auto color = dev.get_frame_data(rs::stream::rectified_color);

            auto dimg = img::Image<uint16_t,1>(w, h, (uint16_t*)disp);
            auto cimg = img::Image<uint8_t,3>(w, h, (uint8_t*)color);

            auto ppdisp = img::domainTransformDepth(dimg, dimg, 3, 13, 16);
            auto ppjoint = img::domainTransformJoint(dimg, cimg, 3, 13, 16*3);

            { //output stuff
                auto himg = img::Image<uint8_t, 3>(w, h);
                auto himg2 = img::Image<uint8_t, 3>(w, h);
                auto himg3 = img::Image<uint8_t, 3>(w, h);
                auto himg4 = img::Image<uint8_t, 3>(w, h);
                auto himg5 = img::Image<uint8_t, 3>(w, h);

                util::ConvertDepthToRGBUsingHistogram(himg.ptr, dimg.ptr, w, h, 0.1f, 0.625f);
                util::ConvertDepthToRGBUsingHistogram(himg2.ptr, ppdisp.ptr, w, h, 0.1f, 0.625f);
                util::ConvertDepthToRGBUsingHistogram(himg4.ptr, ppjoint.ptr, w, h, 0.1f, 0.625f);
                img::imshow("original-c", himg);
                img::imshow("disp-pp-c", himg2);
                img::imshow("joint-pp", himg4);
            }
        }
    } while ( img::getKey() != 'q' );

    return 0;
}