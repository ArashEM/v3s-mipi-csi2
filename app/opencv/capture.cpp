#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

int main(int argc, char** argv)
{
    VideoCapture cap;
    cap.open("/dev/sun6i-isp-capture");

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    cap.set(cv::CAP_PROP_CONVERT_RGB, 0);
    cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('N', 'V', '1', '2'));

    if (!cap.isOpened())
    {
        cerr << "Can't open camera/video stream!" << endl;
        return 1;
    }

    int width = cap.get(CAP_PROP_FRAME_WIDTH);
    int height = cap.get(CAP_PROP_FRAME_HEIGHT);

    cout << "width=" << width << ", height=" << height << endl;

    Mat result;
    Mat frame, reshaped;

    for (size_t cnt = 0; cnt < 10; ++cnt)
    {
        cap >> frame;
        if (frame.empty())
            break;
    }

    cout << frame.cols << "x" << frame.rows << "x" << frame.channels() << std::endl;
    reshaped = frame.reshape(1, height  + height/2);
    cvtColor(reshaped, result, cv::COLOR_YUV2BGR_NV12);
    imwrite("frame.jpg", result);

    return 0;
}