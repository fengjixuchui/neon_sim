// opencv_helper.hpp
// Description: a collection of helper functions when develop with/based on OpenCV
// Author: Zhuo Zhang (imzhuo@foxmail.com)
// Last update: 2022-08-07 19:51:14
//
// Usage:
// #include "opencv_helper.hpp"
// och::dump_mat(mat);
// std::cout << std::boolalpha << och::almostEqual(m1, m2) << std::endl;
// ...

#pragma once

#include <opencv2/core/hal/interface.h>
#include <opencv2/core/traits.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/types_c.h> // CvMat
#include <opencv2/core/core_c.h>  // cvarrToMat
#include <iostream>
#include <string>

namespace och {

//--------------------------------------------------------------------------------
// 1. 打印 Mat 数据类型(包含通道)
//--------------------------------------------------------------------------------
// https://stackoverflow.com/questions/10167534/how-to-find-out-what-type-of-a-mat-object-is-with-mattype-in-opencv
// NOTE: There is `cv::typeToString()` in OpenCV since 3.4.2 and 4.0.0
// May also search the table:
// +--------+----+----+----+----+------+------+------+------+
// |        | C1 | C2 | C3 | C4 | C(5) | C(6) | C(7) | C(8) |
// +--------+----+----+----+----+------+------+------+------+
// | CV_8U  |  0 |  8 | 16 | 24 |   32 |   40 |   48 |   56 |
// | CV_8S  |  1 |  9 | 17 | 25 |   33 |   41 |   49 |   57 |
// | CV_16U |  2 | 10 | 18 | 26 |   34 |   42 |   50 |   58 |
// | CV_16S |  3 | 11 | 19 | 27 |   35 |   43 |   51 |   59 |
// | CV_32S |  4 | 12 | 20 | 28 |   36 |   44 |   52 |   60 |
// | CV_32F |  5 | 13 | 21 | 29 |   37 |   45 |   53 |   61 |
// | CV_64F |  6 | 14 | 22 | 30 |   38 |   46 |   54 |   62 |
// +--------+----+----+----+----+------+------+------+------+
static std::string mat_type_to_string(int type)
{
    std::string r;

    //uchar depth = type & CV_MAT_DEPTH_MASK;
    int depth = CV_MAT_DEPTH(type);

    //uchar channels = 1 + (type >> CV_CN_SHIFT);
    int cn = CV_MAT_CN(type);

    switch (depth)
    {
    case CV_8U: r = "8U"; break;
    case CV_8S: r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    case CV_16F: r = "16F"; break;
    default: r = "User"; break;
    }

    r += "C";
    r += (cn + '0');

    return r;
}

//--------------------------------------------------------------------------------
// 2. 打印 Mat 维度信息、通道信息， stride， step 等
//--------------------------------------------------------------------------------
struct MatMeta
{
    // 基本信息
    int height;
    int width;
    int channels;
    int stride;       // in bytes, 包含了通道在内
    int lineElements; // 按 stride 算出的元素数量. >= width*channels. 用于按行遍历时换行。
    std::string typeStr;

    // 额外信息
    int pixelSize;          // 例如 CV_16SC3，大小是 sizeof(short)*3
    int pixelComponentSize; // 例如 CV_16SC3, 大小是 sizeof(short)。 假定了所有 component 等宽。
};

static MatMeta get_mat_meta(const cv::Mat& mat)
{
    const int line_elements = mat.step1(); // stride / sizeof(T)
    MatMeta meta;
    meta.height = mat.rows;
    meta.width = mat.cols;
    meta.channels = mat.channels();
    meta.stride = mat.step;
    meta.lineElements = mat.step1();
    meta.typeStr = mat_type_to_string(mat.type());

    meta.pixelSize = mat.elemSize();
    meta.pixelComponentSize = mat.elemSize1();

    return meta;
}

static void dump_mat_meta(const cv::Mat& mat)
{
    MatMeta meta = get_mat_meta(mat);
    fprintf(stderr, "height=%d, width=%d, channels=%d, typestr=%s, stride=%d, line_elements=%d\n",
            meta.height, meta.width, meta.channels, meta.typeStr.c_str(), meta.stride, meta.lineElements);
}

//--------------------------------------------------------------------------------
// 3. 比较两个 Mat， 支持多种数据类型
//--------------------------------------------------------------------------------

// 比较一行
template<typename T, typename PT = double>
static bool assertAlmostEqual(const void* pexpected0, const void* pactual0, int y, int cols, int cn, double eps)
{
    const T* expected = (const T*)pexpected0;
    const T* actual = (const T*)pactual0;
    const int len = cols * cn;
    for (int i = 0; i < len; ++i)
    {
        const double diff = std::abs((double)expected[i] - (double)actual[i]);
        if (diff > eps)
        {
            const int x = i / cn;
            const int c = i % cn;
            // fprintf(stderr, "array[%d,%d,%d] (%f) != expected[%d, %d, %d] (%f), diff = %f, EPS = %f\n",
            //     y, x, c, (double)actual[i],
            //     y, x, c, (double)expected[i],
            //     diff, eps
            // );
            std::cerr << "array[" << y << "," << x << "," << c
                      << "] (" << (PT)actual[i] << ") != expected[" << y << "," << x << "," << c
                      << "] (" << (PT)expected[i] << "), diff = " << diff << ", EPS = " << eps << std::endl;

            return false;
        }
    }
    return true;
}

template<typename T, typename PT>
static bool almostEqual(const cv::Mat& expected, const cv::Mat& actual, double eps)
{
    if (expected.type() != actual.type())
    {
        fprintf(stderr, "actual type (%s=%d) is not same as expected type (%s=%d)\n",
                mat_type_to_string(actual.type()).c_str(), actual.type(),
                mat_type_to_string(expected.type()).c_str(), expected.type());
        return false;
    }

    if ((expected.size() != actual.size()))
    {
        fprintf(stderr, "actual size (W=%d, H=%d) is not same as expected size (W=%d, H=%d)\n",
                actual.cols, actual.rows,
                expected.cols, expected.rows);
        return false;
    }

    const int height = expected.rows;
    const int cn = expected.channels();
    for (int y = 0; y < height; ++y)
    {
        auto res = assertAlmostEqual<T, PT>(expected.ptr(y), actual.ptr(y), y, expected.cols, cn, eps);
        if (!res)
            return res;
    }
    return true;
}

static bool almostEqual(const cv::Mat& expected, const cv::Mat& actual, double eps = 0)
{
    if ((expected.rows != actual.rows) || (expected.cols != actual.cols))
    {
        fprintf(stderr, "dims not match: expected(rows=%d, cols=%d) vs actual(rows=%d, cols=%d)\n",
                expected.rows, expected.cols,
                actual.rows, actual.cols);
        return false;
    }
    if (expected.channels() != actual.channels())
    {
        fprintf(stderr, "channels not match: expected(%d) vs actual(%d)\n",
                expected.channels(),
                actual.channels());
        return false;
    }
    if (expected.type() != actual.type())
    {
        fprintf(stderr, "types not match: expected(%s) vs actual(%s)\n",
                mat_type_to_string(expected.type()).c_str(),
                mat_type_to_string(actual.type()).c_str());
        return false;
    }

    uchar depth = CV_MAT_DEPTH(expected.type());
    switch (depth)
    {
    case CV_8U: return almostEqual<uchar, int>(expected, actual, eps);
    case CV_8S: return almostEqual<schar, int>(expected, actual, eps);
    case CV_16U: return almostEqual<ushort, int>(expected, actual, eps);
    case CV_16S: return almostEqual<short, int>(expected, actual, eps);
    case CV_32S: return almostEqual<int, int>(expected, actual, eps);
    case CV_32F: return almostEqual<float, int>(expected, actual, eps);
    case CV_64F: return almostEqual<double, int>(expected, actual, eps);
    case CV_16F: fprintf(stderr, "not implemented\n"); break;
    default: fprintf(stderr, "not implemented\n"); break;
    }

    return false;
}

//--------------------------------------------------------------------------------
// 4. 哈希感知， 比较两个 Mat 的相似性，忽略肉眼看不出来的像素差别
//--------------------------------------------------------------------------------

static void compute_phash(const cv::Mat& src, int feature[64])
{
    cv::Mat small;
    cv::resize(src, small, cv::Size(8, 8));

    cv::Mat gray = small;
    if (small.channels() == 3)
        cv::cvtColor(small, gray, cv::COLOR_BGR2GRAY);
    else if (small.channels() == 4)
        cv::cvtColor(small, gray, cv::COLOR_BGRA2GRAY);

    int sum = 0;
    for (int i = 0; i < 64; i++)
        sum += gray.data[i];
    uint8_t mean = static_cast<uint8_t>(sum * 1.0 / 64);

    memset(feature, 0, sizeof(int) * 64);
    for (int i = 0; i < 64; i++)
        if (gray.data[i] > mean)
            feature[i] = 1;
}

static int compute_hamming_dist(int* feat1, int* feat2, int len)
{
    int dis = 0;
    for (int i = 0; i < len; i++)
        if (feat1[i] != feat2[i])
            dis++;
    return dis;
}

static bool is_hash_similar(const cv::Mat& mat1, const cv::Mat& mat2, int hash_dist_thresh)
{
    if (mat1.depth() != CV_8U || mat2.depth() != CV_8U)
    {
        CV_Error(cv::Error::StsBadArg, "只支持 uchar 类型");
        return false;
    }
    int feature1[64];
    int feature2[64];
    compute_phash(mat1, feature1);
    compute_phash(mat2, feature2);
    int dist = compute_hamming_dist(feature1, feature2, 64);
    //return (dist <= hash_dist_thresh);
    if (dist <= hash_dist_thresh)
    {
        return true;
    }
    else
    {
        fprintf(stderr, "hash dist(%d) > thresh(%d)\n", dist, hash_dist_thresh);
        return false;
    }
}

// 比较两个 Mat 的相同位置像素， 返回最大差异值
template<typename T>
static cv::Scalar auto_compare(const cv::Mat& expected, const cv::Mat& actual)
{
    if ((expected.rows != actual.rows) || (expected.cols != actual.cols))
    {
        fprintf(stderr, "dims not match: expected(rows=%d, cols=%d) vs actual(rows=%d, cols=%d)\n",
                expected.rows, expected.cols,
                actual.rows, actual.cols);
        return cv::Scalar(-1, -1, -1, -1);
    }
    const cv::Size size = expected.size();
    const int channels = expected.channels();
    cv::Scalar max_diff(0, 0, 0, 0);
    cv::Scalar_<std::pair<int, int>> max_diff_pos;
    for (int i = 0; i < size.height; i++)
    {
        for (int j = 0; j < size.width; j++)
        {
            for (int k = 0; k < channels; k++)
            {
                double diff = std::abs(expected.ptr<T>(i, j)[k] != actual.ptr<T>(i, j)[k]);
                if (max_diff.val[k] < diff)
                {
                    max_diff[k] = diff;
                    max_diff_pos[k] = std::make_pair(i, j);
                }
                max_diff.val[k] = std::max(diff, max_diff.val[k]);
            }
        }
    }
    for (int k = 0; k < channels; k++)
    {
        std::cout << max_diff[k] << " in position (" << max_diff_pos[k].first << ", " << max_diff_pos[k].second << ")" << std::endl;
    }
    return max_diff;
}

//--------------------------------------------------------------------------------
// 5. 打印 Mat 元素。通常只适合小尺寸（e.g.10x10 以内）的矩阵
//--------------------------------------------------------------------------------
template<typename T>
static void dump_mat(const cv::Mat& mat)
{
    T* data = (T*)mat.data;
    for (int i = 0; i < mat.rows; i++)
    {
        for (int j = 0; j < mat.cols; j++)
        {
            for (int k = 0; k < mat.channels(); k++)
            {
                int idx = i * mat.step1() + j * mat.channels() + k;
                std::cout << data[idx] << ", ";
            }
            //std::cout << std::endl;
        }
        std::cout << std::endl;
    }
}

template<>
void dump_mat<uint8_t>(const cv::Mat& mat)
{
    using T = uint8_t;
    T* data = (T*)mat.data;
    for (int i = 0; i < mat.rows; i++)
    {
        for (int j = 0; j < mat.cols; j++)
        {
            for (int k = 0; k < mat.channels(); k++)
            {
                std::cout << cv::format("%3d, ", mat.ptr<T>(i, j)[k]);
            }
        }
        std::cout << std::endl;
    }
}

template<typename T>
static void dump_mat2(const cv::Mat& mat)
{
    for (int i = 0; i < mat.rows; i++)
    {
        const int len = mat.cols * mat.channels();
        const T* line = (const T*)mat.ptr(i);
        for (int j = 0; j < len; j++)
        {
            std::cout << line[j] << ", ";
        }
        std::cout << std::endl;
    }
}

static void dump_mat(const cv::Mat& mat)
{
    int type = mat.type();
    uchar depth = type & CV_MAT_DEPTH_MASK;
    switch (depth)
    {
    case CV_8S: dump_mat<schar>(mat); break;
    case CV_8U: dump_mat<uchar>(mat); break;
    case CV_16S: dump_mat<short>(mat); break;
    case CV_16U: dump_mat<ushort>(mat); break;
    case CV_32S: dump_mat<int>(mat); break;
    case CV_32F: dump_mat<float>(mat); break;
    case CV_64F: dump_mat<double>(mat); break;
    default:
        fprintf(stderr, "not supported yet\n");
    }
}

static void dump_mat2(const cv::Mat& mat)
{
    int type = mat.type();
    uchar depth = type & CV_MAT_DEPTH_MASK;
    switch (depth)
    {
    case CV_8S: dump_mat2<schar>(mat); break;
    case CV_8U: dump_mat2<uchar>(mat); break;
    case CV_16S: dump_mat2<short>(mat); break;
    case CV_16U: dump_mat2<ushort>(mat); break;
    case CV_32S: dump_mat2<int>(mat); break;
    case CV_32F: dump_mat2<float>(mat); break;
    case CV_64F: dump_mat2<double>(mat); break;
    default:
        fprintf(stderr, "not supported yet\n");
    }
}

static void cout_mat_example()
{
    cv::Mat kernel = (cv::Mat_<float>(5, 5) << 1, 4, 6, 4, 1,
                      4, 16, 24, 16, 4,
                      6, 24, 36, 24, 6,
                      4, 16, 24, 16, 4,
                      1, 4, 6, 4, 1)
                     / 256;
    std::cout << kernel << std::endl; // 能输出， 但是不好看。

    std::cout << "--------------------" << std::endl;

    // 推荐用 Python 格式的打印
    std::cout << "kernel (numpy) = \n"
              << cv::format(kernel, cv::Formatter::FMT_NUMPY) << ";" << std::endl
              << std::endl;

    // 其他格式
    // std::cout << "kernel (default) = \n" << kernel << ";" << std::endl << std::endl;
    // std::cout << "kernel (matlab) = \n" << format(kernel, cv::Formatter::FMT_MATLAB) << ";" << std::endl << std::endl;
    // std::cout << "kernel (python) = \n" << format(kernel, cv::Formatter::FMT_PYTHON) << ";" << std::endl << std::endl;
    // std::cout << "kernel (csv) = \n" << format(kernel, cv::Formatter::FMT_CSV) << ";" << std::endl << std::endl;
    // std::cout << "kernel (c) = \n" << format(kernel, cv::Formatter::FMT_C) << ";" << std::endl << std::endl;
}

//--------------------------------------------------------------------------------
// 6. 范围截断
// 把 val 截断到闭区间范围 [minval, maxval]
//--------------------------------------------------------------------------------
template<typename T>
static T clip(T val, T minval, T maxval)
{
    if (minval > maxval)
    {
        std::swap(minval, maxval);
    }

    T result = val;
    if (val < minval)
    {
        result = minval;
    }
    if (val > maxval)
    {
        result = maxval;
    }
    return result;
}

// 判断是否在闭区间 [minval, maxval] 范围内
template<typename T>
static bool in_range(T val, T minval, T maxval)
{
    if (minval > maxval)
    {
        std::swap(minval, maxval);
    }

    if (val < minval || val > maxval) return false;
    return true;
}

//--------------------------------------------------------------------------------
// 7. 从逗号和流来初始化 Mat 的例子
// NOTE: 只适用于单通道
//--------------------------------------------------------------------------------
static void comma_and_stream_init_example()
{
    cv::Mat src2 = (cv::Mat_<uchar>(5, 5) << 11, 12, 13, 14, 15,
                    21, 22, 23, 24, 25,
                    31, 32, 33, 34, 35,
                    41, 42, 43, 44, 45,
                    51, 52, 53, 54, 55);
    std::cout << src2 << std::endl;
    dump_mat_meta(src2);
}

static void mat_ptr_example()
{
    cv::Mat kernel = (cv::Mat_<float>(3, 3) << 1, 2, 1,
                      2, 4, 2,
                      1, 2, 1);

    // std::cout << kernel.template ptr<float>(0, 0, 0)[0] << std::endl; // 会导致 crash。单通道Mat，不能有第三个坐标
    // 坑点：Debug版的OpenCV才会执行检查。Release版不检查，可能crash也可能运行通过。开ASan则查到“越界”。

    std::cout << kernel.ptr<float>(0, 0)[0] << std::endl; // ok

    // 对于通道数可能是1，也可能是3的情况，可以这样
    int channels = kernel.channels();
    for (int k = 0; k < channels; k++)
    {
        std::cout << kernel.ptr<float>(0, 0)[k] << std::endl;
    }
}

//--------------------------------------------------------------------------------
// 8. 转换 CvMat 为 Mat
// NOTE: 仅做示例， 请按需修改
//--------------------------------------------------------------------------------
static void cvmat_to_mat_example()
{
    CvMat* matrix = nullptr;
    cv::Mat M0 = cv::cvarrToMat(matrix);
}

//--------------------------------------------------------------------------------
// 9. Logging 例子， 例如参数检查
// CV_StsBadFlag 是宏，是 C API。推荐用 C++ 的 cv::Error::StsBadArg.
//--------------------------------------------------------------------------------
static void logging_example(const cv::Mat& src)
{
    int depth = CV_MAT_DEPTH(src.type());
    if (depth != CV_8U)
    {
        CV_Error(cv::Error::StsBadArg, "只支持 uchar 类型");
    }
    printf("执行良好！\n");
}

//--------------------------------------------------------------------------------
// 10. 打印版本号和构建信息
//--------------------------------------------------------------------------------
static void dump_version_and_build_info()
{
    fprintf(stderr, "OpenCV Version: %d.%d.%d\n",
            CV_VERSION_MAJOR, CV_VERSION_MINOR, CV_VERSION_REVISION);
    fprintf(stderr, "OpenCV Version str: %s\n", cv::getVersionString().c_str());

    printf("OpenCV Build info: %s", cv::getBuildInformation().c_str());
}

//--------------------------------------------------------------------------------
// 11. 用 NORM_MINMAX 方式归一化 16UC1 图像到 8UC1 图像
// imshow 不能正常显示深度图， 归一化后可显示
//--------------------------------------------------------------------------------
static void normalize_16UC1_to_8UC1(const cv::Mat& src, cv::Mat& normed)
{
    double minval;
    double maxval;
    cv::minMaxLoc(src, &minval, &maxval);

    normed.create(src.size(), CV_8UC1);
    int height = src.size().height;
    int width = src.size().width;
    uint16_t minval_u16 = (uint16_t)minval;
    double diff = maxval - minval;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            normed.ptr<uchar>(i, j)[0] = 255 * ((src.ptr<uint16_t>(i, j)[0] - minval_u16) / diff);
        }
    }
}

//--------------------------------------------------------------------------------
// 12. 显示 depth 图的例子
//--------------------------------------------------------------------------------
static void show_depth_image_example()
{
#if __ANDROID__
    std::string load_prefix = "/data/local/tmp";
#elif __linux__
    std::string load_prefix = "/home/zz/data";
#elif _MSC_VER
    std::string load_prefix = "d:/data";
#elif __APPLE__
    std::string load_prefix = "/Users/chris/data";
#else
#pragma error
#endif

    std::string image_path = load_prefix + "/depth_image.png";
    cv::Mat image = cv::imread(image_path, cv::IMREAD_UNCHANGED);
    cv::Mat normed_image;
    normalize_16UC1_to_8UC1(image, normed_image);
    cv::imshow("image", image);
    cv::imshow("normed_image", normed_image);
    cv::waitKey(0);
}

//--------------------------------------------------------------------------------
// 13. NV21/NV12 和 RGB/BGR 的相互转换例子
//--------------------------------------------------------------------------------
static void convert_nv21_to_bgr_method1(const cv::Mat& y_plane, const cv::Mat& uv_plane, cv::Mat& bgr)
{
    cv::cvtColorTwoPlane(y_plane, uv_plane, bgr, cv::COLOR_YUV2BGR_NV21);
}

static void convert_nv21_to_bgr_method2()
{
    // 准备数据， 模拟用户输入
    const int h = 480;
    const int w = 640;
    const int bufsize = h * w * 3 / 2;
    uint8_t* yuv_buf = (uint8_t*)malloc(bufsize);

    // 把 Y 和 UV （连续的两个平面） 放在一个 Mat 里， 设置通道数量为1
    cv::Mat mat_src = cv::Mat(h * 3 / 2, w, CV_8UC1, yuv_buf);
    cv::Mat mat_dst = cv::Mat(h, w, CV_8UC3);
    cv::cvtColor(mat_src, mat_dst, cv::COLOR_YUV2BGR_NV21);
    cv::imwrite("result_cvtColor.png", mat_dst);

    free(yuv_buf);
}

static void convert_bgr_to_nv21(const cv::Mat& bgr, cv::Mat& y_plane, cv::Mat& uv_plane)
{
    //TODO: opencv 不支持这个转换。。
}

//--------------------------------------------------------------------------------
// 14. 根据 uint8_t 等基本类型， 编译期获得 TV_8U 等类型, 在模板函数中常用。
//--------------------------------------------------------------------------------
// depth => type
static void use_data_type_example()
{
    cv::Mat src0(cv::Size(30, 30), CV_8UC3);
    cv::Mat src1(cv::Size(30, 30), cv::DataType<uint8_t>::type);                                                    // 8UC1
    cv::Mat src2(cv::Size(30, 30), cv::DataType<uint8_t>::channel_type(3));                                         // 16SC1
    cv::Mat src3(cv::Size(30, 30), cv::DataType<uint8_t>::depth);                                                   // 8UC1
    cv::Mat src4(cv::Size(30, 30), cv::DataType<uint8_t>::work_type());                                             // 8UC1
    cv::Mat src5(cv::Size(30, 30), CV_MAKETYPE(cv::DataType<uint8_t>::depth, cv::DataType<uint8_t>::channels * 3)); // 8UC3
    cv::Mat src6(cv::Size(30, 30), CV_MAKETYPE(cv::DataType<uint8_t>::depth, 3));                                   // 8UC3
    // 注：在ToyCV中，DataType<T>::channels仅在单通道时等于1.
    // 多通道情况下如 complex 类型， channels = 2
    // template<typename T> struct DataType< Complex<T> >

    cv::Mat src7(cv::Size(30, 30), CV_MAKETYPE(cv::DataType<uint16_t>::depth, cv::DataType<uint8_t>::channels * 3)); // 16UC3
    cv::Mat src8(cv::Size(30, 30), CV_MAKETYPE(cv::DataType<uint16_t>::depth, 3));                                   // 16UC3

    dump_mat_meta(src0);
    dump_mat_meta(src1);
    dump_mat_meta(src2);
    dump_mat_meta(src3);
    dump_mat_meta(src4);
    dump_mat_meta(src5);
    dump_mat_meta(src6);
    dump_mat_meta(src7);
    dump_mat_meta(src8);
}

// type => depth
static void mat_type_to_depth_example()
{
    cv::Mat src = cv::Mat(256, 256, CV_8UC3);
    const int type = src.type();
    const int depth = CV_MAT_DEPTH(type);
}

//--------------------------------------------------------------------------------
// 15. 颜色
//--------------------------------------------------------------------------------

// 根据16进制数字，获取对应的颜色表示
static
cv::Scalar get_bgr_color_from_hex(int hex)
{
    int b = hex / 65536;
    int g = (hex % 65536) / 256;
    int r = hex % 256;
    return cv::Scalar(b, g, r);
}

static
cv::Scalar get_rgb_color_from_hex(int hex)
{
    int b = hex / 65536;
    int g = (hex % 65536) / 256;
    int r = hex % 256;
    return cv::Scalar(r, g, b);
}

//--------------------------------------------------------------------------------
// 16. 几何计算，几何变换相关
//--------------------------------------------------------------------------------

// 角度(radian, angle)和弧度(degrees)互相转换
template<typename T>
static T radian_to_degree(T v)
{
    return v * 180.0 / CV_PI;
}

template<typename T>
static T degree_to_radian(T v)
{
    return v * CV_PI / 180.0;
}


/// @brief Rotate a point with respect to an angle and a Point
/// @details Due to image's orginal point(0, 0) is top-left instead of bottom-left, 
/// we require @im_h, the image height, for correct image corrodinate computation.
/// @param sp source point
/// @param center Rotate center point
/// @param im_h image height
/// @param theta angle in rad (弧度)
static
cv::Point get_rotated_point(cv::Point src, cv::Point center, int im_h, float theta)
{
    float x1 = src.x;
    float y1 = src.y;
    float x2 = center.x;
    float y2 = center.y;

    y1 = im_h - y1;
    y2 = im_h - y2;
    float dx = (x1 - x2)*cos(theta) - (y1 - y2)*sin(theta) + x2;
    float dy = (x1 - x2)*sin(theta) + (y1 - y2)*cos(theta) + y2;
    dy = im_h - dy;

    cv::Point dst(dx, dy);
    return dst;
}

/// @brief 获取正多边形定点的坐标
/// @param image_size The size of image
/// @param n n should be >= 3 && <= 50
static
std::vector<cv::Point> get_regular_polygon_vertices(cv::Size image_size, int n, int radius = 100)
{
    const int w = image_size.width;
    const int h = image_size.height;
    cv::Point center = image_size / 2;

    std::vector<cv::Point> pts;
    if (n < 3 || n >= 50)
    {
        return pts;
    }
    // the plotted polygon should not out of boundary
    radius = std::min(radius, std::min(w, h) / 2);

    // take the triangle for inspiration
    //      A
    //
    //      O
    //
    // B         C

    cv::Point O(center.x, center.y);

    float rotate_angle = 360.0 / n;
    int offy = radius;
    cv::Point A(O.x, O.y - offy);
    for (int i = 0; i < n; i++)
    {
        float theta = och::degree_to_radian(rotate_angle * i);
        cv::Point pt = och::get_rotated_point(A, center, h, theta);
        pts.push_back(pt);
    }
    return pts;
}


} // namespace och

//--------------------------------------------------------------------------------
// 16. 低版本的宏转换为高版本的枚举元素
//--------------------------------------------------------------------------------
// opencv2.4 => opencv3.x: https://docs.opencv.org/4.x/db/dfa/tutorial_transition_guide.html

//#define DO_CV_MACRO_AS_ENUM 1
#if CV_MAJOR_VERSION >= 4 && DO_CV_MACRO_AS_ENUM

// enum InterpolationFlags
#define CV_INTER_NN       cv::INTER_NN
#define CV_INTER_LINEAR   cv::INTER_LINEAR
#define CV_INTER_CUBIC    cv::INTER_CUBIC
#define CV_INTER_AREA     cv::INTER_AREA
#define CV_INTER_LANCZOS4 cv::INTER_LANCZOS4

// enum ColorConversionCodes
#define CV_BGR2BGRA cv::COLOR_BGR2BGRA
#define CV_RGB2RGBA cv::COLOR_CV_RGB2RGBA

#define CV_BGRA2BGR cv::COLOR_BGRA2BGR
#define CV_RGBA2RGB cv::COLOR_RGBA2RGB

#define CV_BGR2RGBA cv::COLOR_BGR2RGBA
#define CV_RGB2BGRA cv::COLOR_RGB2BGRA

#define CV_RGBA2BGR cv::COLOR_RGBA2BGR
#define CV_BGRA2RGB cv::COLOR_BGRA2RGB

#define CV_BGR2RGB cv::COLOR_BGR2RGB
#define CV_RGB2BGR cv::COLOR_RGB2BGR

#define CV_BGRA2RGBA cv::COLOR_BGRA2RGBA
#define CV_RGBA2BGRA cv::COLOR_RGBA2BGRA

#define CV_BGR2GRAY  cv::COLOR_BGR2GRAY
#define CV_RGB2GRAY  cv::COLOR_RGB2GRAY
#define CV_GRAY2BGR  cv::COLOR_GRAY2BGR
#define CV_GRAY2RGB  cv::COLOR_GRAY2RGB
#define CV_GRAY2BGRA cv::COLOR_GRAY2BGRA
#define CV_GRAY2RGBA cv::COLOR_GRAY2RGBA
#define CV_BGRA2GRAY cv::COLOR_BGRA2GRAY
#define CV_RGBA2GRAY cv::COLOR_RGBA2GRAY

#define CV_BGR2BGR565  cv::COLOR_BGR2BGR565
#define CV_RGB2BGR565  cv::COLOR_RGB2BGR565
#define CV_BGR5652BGR  cv::COLOR_BGR5652BGR
#define CV_BGR5652RGB  cv::COLOR_BGR5652RGB
#define CV_BGRA2BGR565 cv::COLOR_BGRA2BGR565
#define CV_RGBA2BGR565 cv::COLOR_RGBA2BGR565
#define CV_BGR5652BGRA cv::COLOR_BGR5652BGRA
#define CV_BGR5652RGBA cv::COLOR_BGR5652RGBA

#define CV_GRAY2BGR565 cv::COLOR_GRAY2BGR565
#define CV_BGR5652GRAY cv::COLOR_BGR5652GRAY

#define CV_BGR2BGR555  cv::COLOR_BGR2BGR555
#define CV_RGB2BGR555  cv::COLOR_RGB2BGR555
#define CV_BGR5552BGR  cv::COLOR_BGR5552BGR
#define CV_BGR5552RGB  cv::COLOR_BGR5552RGB
#define CV_BGRA2BGR555 cv::COLOR_BGRA2BGR555
#define CV_RGBA2BGR555 cv::COLOR_RGBA2BGR555
#define CV_BGR5552BGRA cv::COLOR_BGR5552BGRA
#define CV_BGR5552RGBA cv::COLOR_BGR5552RGBA

#define CV_GRAY2BGR555 cv::COLOR_GRAY2BGR555
#define CV_BGR5552GRAY cv::COLOR_BGR5552GRAY

#define CV_BGR2XYZ cv::COLOR_BGR2XYZ
#define CV_RGB2XYZ cv::COLOR_RGB2XYZ
#define CV_XYZ2BGR cv::COLOR_XYZ2BGR
#define CV_XYZ2RGB cv::COLOR_XYZ2RGB

#define CV_BGR2YCrCb cv::COLOR_BGR2YCrCb
#define CV_RGB2YCrCb cv::COLOR_RGB2YCrCb
#define CV_YCrCb2BGR cv::COLOR_YCrCb2BGR
#define CV_YCrCb2RGB cv::COLOR_YCrCb2RGB

#define CV_BGR2HSV cv::COLOR_BGR2HSV
#define CV_RGB2HSV cv::COLOR_RGB2HSV

#define CV_BGR2Lab cv::COLOR_BGR2Lab
#define CV_RGB2Lab cv::COLOR_RGB2Lab

#define CV_BayerBG2BGR cv::COLOR_BayerBG2BGR
#define CV_BayerGB2BGR cv::COLOR_BayerGB2BGR
#define CV_BayerRG2BGR cv::COLOR_BayerRG2BGR
#define CV_BayerGR2BGR cv::COLOR_BayerGR2BGR

#define CV_BayerBG2RGB cv::COLOR_BayerBG2RGB
#define CV_BayerGB2RGB cv::COLOR_BayerGB2RGB
#define CV_BayerRG2RGB cv::COLOR_BayerRG2RGB
#define CV_BayerGR2RGB cv::COLOR_BayerGR2RGB

#define CV_BGR2Luv cv::COLOR_BGR2Luv
#define CV_RGB2Luv cv::COLOR_RGB2Luv
#define CV_BGR2HLS cv::COLOR_BGR2HLS
#define CV_RGB2HLS cv::COLOR_RGB2HLS

#define CV_HSV2BGR cv::COLOR_HSV2BGR
#define CV_HSV2RGB cv::COLOR_HSV2RGB

#define CV_Lab2BGR cv::COLOR_Lab2BGR
#define CV_Lab2RGB cv::COLOR_Lab2RGB
#define CV_Luv2BGR cv::COLOR_Luv2BGR
#define CV_Luv2RGB cv::COLOR_Luv2RGB
#define CV_HLS2BGR cv::COLOR_HLS2BGR
#define CV_HLS2RGB cv::COLOR_HLS2RGB

#define CV_BayerBG2BGR_VNG cv::COLOR_BayerBG2BGR_VNG
#define CV_BayerGB2BGR_VNG cv::COLOR_BayerGB2BGR_VNG
#define CV_BayerRG2BGR_VNG cv::COLOR_BayerRG2BGR_VNG
#define CV_BayerGR2BGR_VNG cv::COLOR_BayerGR2BGR_VNG

#define CV_BayerBG2RGB_VNG cv::COLOR_BayerBG2RGB_VNG
#define CV_BayerGB2RGB_VNG cv::COLOR_BayerGB2RGB_VNG
#define CV_BayerRG2RGB_VNG cv::COLOR_BayerRG2RGB_VNG
#define CV_BayerGR2RGB_VNG cv::COLOR_BayerGR2RGB_VNG

#define CV_BGR2HSV_FULL cv::COLOR_BGR2HSV_FULL
#define CV_RGB2HSV_FULL cv::COLOR_RGB2HSV_FULL
#define CV_BGR2HLS_FULL cv::COLOR_BGR2HLS_FULL
#define CV_RGB2HLS_FULL cv::COLOR_RGB2HLS_FULL

#define CV_HSV2BGR_FULL cv::COLOR_HSV2BGR_FULL
#define CV_HSV2RGB_FULL cv::COLOR_HSV2RGB_FULL
#define CV_HLS2BGR_FULL cv::COLOR_HLS2BGR_FULL
#define CV_HLS2RGB_FULL cv::COLOR_HLS2RGB_FULL

#define CV_LBGR2Lab cv::COLOR_LBGR2Lab
#define CV_LRGB2Lab cv::COLOR_LRGB2Lab
#define CV_LBGR2Luv cv::COLOR_LBGR2Luv
#define CV_LRGB2Luv cv::COLOR_LRGB2Luv

#define CV_Lab2LBGR cv::COLOR_Lab2LBGR
#define CV_Lab2LRGB cv::COLOR_Lab2LRGB
#define CV_Luv2LBGR cv::COLOR_Luv2LBGR
#define CV_Luv2LRGB cv::COLOR_Luv2LRGB

#define CV_BGR2YUV cv::COLOR_BGR2YUV
#define CV_RGB2YUV cv::COLOR_RGB2YUV
#define CV_YUV2BGR cv::COLOR_YUV2BGR
#define CV_YUV2RGB cv::COLOR_YUV2RGB

#define CV_BayerBG2GRAY cv::COLOR_BayerBG2GRAY
#define CV_BayerGB2GRAY cv::COLOR_BayerGB2GRAY
#define CV_BayerRG2GRAY cv::COLOR_BayerRG2GRAY
#define CV_BayerGR2GRAY cv::COLOR_BayerGR2GRAY

#define CV_YUV2RGB_NV12 cv::COLOR_YUV2RGB_NV12
#define CV_YUV2BGR_NV12 cv::COLOR_YUV2BGR_NV12
#define CV_YUV2RGB_NV21 cv::COLOR_YUV2RGB_NV21
#define CV_YUV2BGR_NV21 cv::COLOR_YUV2BGR_NV21
#define CV_YUV420sp2RGB cv::COLOR_YUV420sp2RGB
#define CV_YUV420sp2BGR cv::COLOR_YUV420sp2BGR

#define CV_YUV2RGBA_NV12 cv::COLOR_YUV2RGBA_NV12
#define CV_YUV2BGRA_NV12 cv::COLOR_YUV2BGRA_NV12
#define CV_YUV2RGBA_NV21 cv::COLOR_YUV2RGBA_NV21
#define CV_YUV2BGRA_NV21 cv::COLOR_YUV2BGRA_NV21
#define CV_YUV420sp2RGBA cv::COLOR_YUV420sp2RGBA
#define CV_YUV420sp2BGRA cv::COLOR_YUV420sp2BGRA

#define CV_YUV2RGB_YV12 cv::COLOR_YUV2RGB_YV12
#define CV_YUV2BGR_YV12 cv::COLOR_YUV2BGR_YV12
#define CV_YUV2RGB_IYUV cv::COLOR_YUV2RGB_IYUV
#define CV_YUV2BGR_IYUV cv::COLOR_YUV2BGR_IYUV
#define CV_YUV2RGB_I420 cv::COLOR_YUV2RGB_I420
#define CV_YUV2BGR_I420 cv::COLOR_YUV2BGR_I420
#define CV_YUV420p2RGB  cv::COLOR_YUV420p2RGB
#define CV_YUV420p2BGR  cv::COLOR_YUV420p2BGR

#define CV_YUV2RGBA_YV12 cv::COLOR_YUV2RGBA_YV12
#define CV_YUV2BGRA_YV12 cv::COLOR_YUV2BGRA_YV12
#define CV_YUV2RGBA_IYUV cv::COLOR_YUV2RGBA_IYUV
#define CV_YUV2BGRA_IYUV cv::COLOR_YUV2BGRA_IYUV
#define CV_YUV2RGBA_I420 cv::COLOR_YUV2RGBA_I420
#define CV_YUV2BGRA_I420 cv::COLOR_YUV2BGRA_I420
#define CV_YUV420p2RGBA  cv::COLOR_YUV420p2RGBA
#define CV_YUV420p2BGRA  cv::COLOR_YUV420p2BGRA

#define CV_YUV2GRAY_420  cv::COLOR_YUV2GRAY_420
#define CV_YUV2GRAY_NV21 cv::COLOR_YUV2GRAY_NV21
#define CV_YUV2GRAY_NV12 cv::COLOR_YUV2GRAY_NV12
#define CV_YUV2GRAY_YV12 cv::COLOR_YUV2GRAY_YV12
#define CV_YUV2GRAY_IYUV cv::COLOR_YUV2GRAY_IYUV
#define CV_YUV2GRAY_I420 cv::COLOR_YUV2GRAY_I420
#define CV_YUV420sp2GRAY cv::COLOR_YUV420sp2GRAY
#define CV_YUV420p2GRAY  cv::COLOR_YUV420p2GRAY

#define CV_YUV2RGB_UYVY cv::COLOR_YUV2RGB_UYVY
#define CV_YUV2BGR_UYVY cv::COLOR_YUV2BGR_UYVY

#define CV_YUV2RGB_Y422 cv::COLOR_YUV2RGB_Y422
#define CV_YUV2BGR_Y422 cv::COLOR_YUV2BGR_Y422
#define CV_YUV2RGB_UYNV cv::COLOR_YUV2RGB_UYNV
#define CV_YUV2BGR_UYNV cv::COLOR_YUV2BGR_UYNV

#define CV_YUV2RGBA_UYVY cv::COLOR_YUV2RGBA_UYVY
#define CV_YUV2BGRA_UYVY cv::COLOR_YUV2BGRA_UYVY

#define CV_YUV2RGBA_Y422 cv::COLOR_YUV2RGBA_Y422
#define CV_YUV2BGRA_Y422 cv::COLOR_YUV2BGRA_Y422
#define CV_YUV2RGBA_UYNV cv::COLOR_YUV2RGBA_UYNV
#define CV_YUV2BGRA_UYNV cv::COLOR_YUV2BGRA_UYNV

#define CV_YUV2RGB_YUY2 cv::COLOR_YUV2RGB_YUY2
#define CV_YUV2BGR_YUY2 cv::COLOR_YUV2BGR_YUY2
#define CV_YUV2RGB_YVYU cv::COLOR_YUV2RGB_YVYU
#define CV_YUV2BGR_YVYU cv::COLOR_YUV2BGR_YVYU
#define CV_YUV2RGB_YUYV cv::COLOR_YUV2RGB_YUYV
#define CV_YUV2BGR_YUYV cv::COLOR_YUV2BGR_YUYV
#define CV_YUV2RGB_YUNV cv::COLOR_YUV2RGB_YUNV
#define CV_YUV2BGR_YUNV cv::COLOR_YUV2BGR_YUNV

#define CV_YUV2RGBA_YUY2 cv::COLOR_YUV2RGBA_YUY2
#define CV_YUV2BGRA_YUY2 cv::COLOR_YUV2BGRA_YUY2
#define CV_YUV2RGBA_YVYU cv::COLOR_YUV2RGBA_YVYU
#define CV_YUV2BGRA_YVYU cv::COLOR_YUV2BGRA_YVYU
#define CV_YUV2RGBA_YUYV cv::COLOR_YUV2RGBA_YUYV
#define CV_YUV2BGRA_YUYV cv::COLOR_YUV2BGRA_YUYV
#define CV_YUV2RGBA_YUNV cv::COLOR_YUV2RGBA_YUNV
#define CV_YUV2BGRA_YUNV cv::COLOR_YUV2BGRA_YUNV

#define CV_YUV2GRAY_UYVY cv::COLOR_YUV2GRAY_UYVY
#define CV_YUV2GRAY_YUY2 cv::COLOR_YUV2GRAY_YUY2
#define CV_YUV2GRAY_Y422 cv::COLOR_YUV2GRAY_Y422
#define CV_YUV2GRAY_UYNV cv::COLOR_YUV2GRAY_UYNV
#define CV_YUV2GRAY_YVYU cv::COLOR_YUV2GRAY_YVYU
#define CV_YUV2GRAY_YUYV cv::COLOR_YUV2GRAY_YUYV
#define CV_YUV2GRAY_YUNV cv::COLOR_YUV2GRAY_YUNV

#define CV_RGBA2mRGBA cv::COLOR_RGBA2mRGBA
#define CV_mRGBA2RGBA cv::COLOR_mRGBA2RGBA

#define CV_RGB2YUV_I420 cv::COLOR_RGB2YUV_I420
#define CV_BGR2YUV_I420 cv::COLOR_BGR2YUV_I420
#define CV_RGB2YUV_IYUV cv::COLOR_RGB2YUV_IYUV
#define CV_BGR2YUV_IYUV cv::COLOR_BGR2YUV_IYUV

#define CV_RGBA2YUV_I420 cv::COLOR_RGBA2YUV_I420
#define CV_BGRA2YUV_I420 cv::COLOR_BGRA2YUV_I420
#define CV_RGBA2YUV_IYUV cv::COLOR_RGBA2YUV_IYUV
#define CV_BGRA2YUV_IYUV cv::COLOR_BGRA2YUV_IYUV
#define CV_RGB2YUV_YV12  cv::COLOR_RGB2YUV_YV12
#define CV_BGR2YUV_YV12  cv::COLOR_BGR2YUV_YV12
#define CV_RGBA2YUV_YV12 cv::COLOR_RGBA2YUV_YV12
#define CV_BGRA2YUV_YV12 cv::COLOR_BGRA2YUV_YV12

#define CV_COLORCVT_MAX cv::COLOR_COLORCVT_MAX

// enum ImreadModes
#define CV_IMREAD_COLOR               cv::IMREAD_COLOR
#define CV_IMREAD_UNCHANGED           cv::IMREAD_UNCHANGED
#define CV_IMREAD_GRAYSCALE           cv::IMREAD_GRAYSCALE
#define CV_IMREAD_COLOR               cv::IMREAD_COLOR
#define CV_IMREAD_ANYDEPTH            cv::IMREAD_ANYDEPTH
#define CV_IMREAD_ANYCOLOR            cv::IMREAD_ANYCOLOR
#define CV_IMREAD_LOAD_GDAL           cv::IMREAD_LOAD_GDAL
#define CV_IMREAD_REDUCED_GRAYSCALE_2 cv::IMREAD_REDUCED_GRAYSCALE_2
#define CV_IMREAD_REDUCED_COLOR_2     cv::IMREAD_REDUCED_COLOR_2
#define CV_IMREAD_REDUCED_GRAYSCALE_4 cv::IMREAD_REDUCED_GRAYSCALE_4
#define CV_IMREAD_REDUCED_COLOR_4     cv::IMREAD_REDUCED_COLOR_4
#define CV_IMREAD_REDUCED_GRAYSCALE_8 cv::IMREAD_REDUCED_GRAYSCALE_8
#define CV_IMREAD_REDUCED_COLOR_8     cv::IMREAD_REDUCED_COLOR_8
#define CV_IMREAD_IGNORE_ORIENTATION  cv::IMREAD_IGNORE_ORIENTATION

// enum ThresholdTypes
#define CV_THRESH_BINARY     cv::THRESH_BINARY
#define CV_THRESH_BINARY_INV cv::THRESH_BINARY_INV
#define CV_THRESH_TRUNC      cv::THRESH_TRUNC
#define CV_THRESH_TOZERO     cv::THRESH_TOZERO
#define CV_THRESH_TOZERO_INV cv::THRESH_TOZERO_INV
#define CV_THRESH_MASK       cv::THRESH_MASK
#define CV_THRESH_OTSU       cv::THRESH_OTSU
#define CV_THRESH_TRIANGLE   cv::THRESH_TRIANGLE

// enum RetrievalModes
#define CV_RETR_EXTERNAL  cv::RETR_EXTERNAL
#define CV_RETR_LIST      cv::RETR_LIST
#define CV_RETR_CCOMP     cv::RETR_CCOMP
#define CV_RETR_TREE      cv::RETR_TREE
#define CV_RETR_FLOODFILL cv::RETR_FLOODFILL

// enum ContourApproximationModes
#define CV_CHAIN_APPROX_NONE      cv::CHAIN_APPROX_NONE
#define CV_CHAIN_APPROX_SIMPLE    cv::CHAIN_APPROX_SIMPLE
#define CV_CHAIN_APPROX_TC89_L1   cv::CHAIN_APPROX_TC89_L1
#define CV_CHAIN_APPROX_TC89_KCOS cv::CHAIN_APPROX_TC89_KCOS

// enum HoughModes
#define CV_HOUGH_STANDARD      cv::HOUGH_STANDARD
#define CV_HOUGH_PROBABILISTIC cv::HOUGH_PROBABILISTIC
#define CV_HOUGH_MULTI_SCALE   cv::HOUGH_MULTI_SCALE
#define CV_HOUGH_GRADIENT      cv::HOUGH_GRADIENT

// enum VideoCaptureProperties
#define CV_CAP_PROP_FRAME_WIDTH  cv::CAP_PROP_FRAME_WIDTH
#define CV_CAP_PROP_FRAME_HEIGHT cv::CAP_PROP_FRAME_HEIGHT
#define CV_CAP_PROP_FPS          cv::CAP_PROP_FPS
#define CV_CAP_PROP_FOURCC       cv::CAP_PROP_FOURCC

// misc
#define CV_LOAD_IMAGE_COLOR     cv::IMREAD_COLOR
#define CV_LOAD_IMAGE_GRAYSCALE cv::IMREAD_GRAYSCALE
#define CV_BGR2HSV              cv::COLOR_BGR2HSV
#define CV_BGR2GRAY             cv::COLOR_BGR2GRAY
#define CV_AA                   cv::LINE_AA

#endif // CV_MAJOR_VERSION>=4 && DO_CV_MACRO_AS_ENUM
