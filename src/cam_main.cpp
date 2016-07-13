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

const int dtIters = 3;
const float dtSpace = 13;
const float dtRange = 16;

img::Img<uint16_t> processDepth(img::Img<uint16_t> depth)
{
    return img::domainTransformDepth(depth, depth, dtIters, dtSpace, dtRange);
}

img::Img<uint16_t> processFillDepth(img::Img<uint16_t> depth)
{
    auto dfill = depth.copy(); left_fill(dfill.ptr, depth.width, depth.height);
    return img::domainTransformDepth(dfill, dfill, dtIters, dtSpace, dtRange);
}

img::Img<uint16_t> processFillColorize(img::Img<uint16_t> depth)
{
    auto dfill_zero = depth.copy();
    img::Img<float> zeromask(depth.width, depth.height);
    for (int i = 0; i < depth.width*depth.height; i++) {
        dfill_zero.ptr[i] = (dfill_zero.ptr[i] == USHRT_MAX) ? 0 : dfill_zero.ptr[i];
        zeromask.ptr[i] = (dfill_zero.ptr[i]) ? 1.0f : 0.0f;
    }
    auto ppfill_z = img::domainTransform(dfill_zero, dfill_zero, dtIters, dtSpace, dtRange);
    auto ppfill_o = img::domainTransform(zeromask, zeromask, dtIters, dtSpace, dtRange);

    for (int i = 0; i < depth.width*depth.height; i++) {
        ppfill_z.ptr[i] = static_cast<uint16_t>(ppfill_z.ptr[i] / ppfill_o.ptr[i]);
    }
    return ppfill_z;
}

img::Img<uint16_t> processDepthJoint(img::Img<uint16_t> depth, img::Image<uint8_t, 3> color)
{
    return img::domainTransformJoint(depth, color, dtIters, dtSpace, 3 * dtIters);
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
    auto intrinC = dev.get_stream_intrinsics(rs::stream::rectified_color);

    auto extrin = dev.get_extrinsics(rs::stream::infrared2, rs::stream::infrared);

    auto fB = intrin.fx * extrin.translation[0] * 1000.0f;
    int counter = 0;

    auto vizD1 = img::Image<uint8_t, 3>(intrin.width, intrin.height);
    auto vizD2 = img::Image<uint8_t, 3>(intrin.width, intrin.height);
    auto vizD3 = img::Image<uint8_t, 3>(intrin.width, intrin.height);
    auto vizC1 = img::Image<uint8_t, 3>(intrinC.width, intrinC.height);
    auto vizC2 = img::Image<uint8_t, 3>(intrinC.width, intrinC.height);
    auto vizC3 = img::Image<uint8_t, 3>(intrinC.width, intrinC.height);
    do {
        dev.wait_for_frames();
        //depth based
        {
            auto w = dev.get_stream_width(rs::stream::depth);
            auto h = dev.get_stream_height(rs::stream::depth);

            auto disp = dev.get_frame_data(rs::stream::depth);

            auto dimg = img::Img<uint16_t>(w, h, (uint16_t*)disp);
            
            auto ppdisp = processDepth(dimg);
            auto ppfill = processFillDepth(dimg);
            //auto ppclrz = processFillColorize(dimg);

            { //output stuff
                util::ConvertDepthToRGBUsingHistogram(vizD1.ptr, dimg.ptr, w, h, 0.1f, 0.625f);
                util::ConvertDepthToRGBUsingHistogram(vizD2.ptr, ppdisp.ptr, w, h, 0.1f, 0.625f);
                util::ConvertDepthToRGBUsingHistogram(vizD3.ptr, ppfill.ptr, w, h, 0.1f, 0.625f);
                
                img::imshow("original", vizD1);
                img::imshow("disp-pp", vizD2);
                img::imshow("disp-fill-pp", vizD3);
            }
        }
        { // color based
            auto w = dev.get_stream_width(rs::stream::rectified_color);
            auto h = dev.get_stream_height(rs::stream::rectified_color);

            auto disp = dev.get_frame_data(rs::stream::depth_aligned_to_rectified_color);
            auto color = dev.get_frame_data(rs::stream::rectified_color);

            auto dimg = img::Image<uint16_t,1>(w, h, (uint16_t*)disp);
            auto cimg = img::Image<uint8_t,3>(w, h, (uint8_t*)color);

            auto ppdisp = processDepth(dimg);
            auto ppjoint = processDepthJoint(dimg, cimg);

            { //output stuff
                util::ConvertDepthToRGBUsingHistogram(vizC1.ptr, dimg.ptr, w, h, 0.1f, 0.625f);
                util::ConvertDepthToRGBUsingHistogram(vizC2.ptr, ppdisp.ptr, w, h, 0.1f, 0.625f);
                util::ConvertDepthToRGBUsingHistogram(vizC3.ptr, ppjoint.ptr, w, h, 0.1f, 0.625f);
                img::imshow("original-c", vizC1);
                img::imshow("disp-pp-c", vizC2);
                img::imshow("joint-pp-c", vizC3);
            }
        }
    } while ( img::getKey() != 'q' );

    return 0;
}