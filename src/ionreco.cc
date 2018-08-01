#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <sstream>
#include <ctime>
#include <sys/time.h>
#include <webcam.h>
using namespace cv;
using namespace std;
const int dev_num = 0;
const int fps = 15;
const int scale = 2; // Resizing factor.  Applied for only display, not recording.

void SetControl(CHandle handle, CControlId id, int value_new);

int main(int, char**)
{
   CResult ret;
   CHandle handle;
   //CControlValue value;
   
   VideoCapture cap(dev_num);
   if (!cap.isOpened()) return -1;
   Size cap_size (1280, 720);
   cap.set (CV_CAP_PROP_FPS         , fps            );
   cap.set (CV_CAP_PROP_FRAME_WIDTH , cap_size.width );
   cap.set (CV_CAP_PROP_FRAME_HEIGHT, cap_size.height);

   Size rec_size(400, 400); // Record only this center part of cap_size.
   int x0 = (cap_size.width  - rec_size.width ) / 2;
   int y0 = (cap_size.height - rec_size.height) / 2;
   
   time_t utime = time(0);
   tm* ltime = localtime(&utime);
   char fname[256];
   strftime(fname, sizeof(fname),"iontrap-%Y-%m-%d--%H-%M-%S.avi", ltime);
   cv::VideoWriter writer(fname, CV_FOURCC('M','J','P','G'), fps, rec_size);
   namedWindow("frame");
   
   ret = c_init();
   if (ret) {
      cout << "Error : c_init() : " << ret << "\n";
      return 1;
   }
   
   ostringstream oss;
   oss << "video" << dev_num;
   handle = c_open_device(oss.str().c_str());
   if (!handle) {
      cout << "Error : c_open_device()\n";
      c_cleanup();
      return 1;
   }

   SetControl(handle, CC_BRIGHTNESS    , 120);
   SetControl(handle, CC_CONTRAST      ,  50);
   SetControl(handle, CC_SATURATION    ,  20);
   SetControl(handle, CC_SHARPNESS     ,  50);
   SetControl(handle, CC_AUTO_FOCUS    ,   0);
   SetControl(handle, CC_FOCUS_ABSOLUTE, 255);
   SetControl(handle, CC_ZOOM_ABSOLUTE ,   2);
   SetControl(handle, CC_POWER_LINE_FREQUENCY, 1); // 1 = 50 Hz
   SetControl(handle, CC_LOGITECH_LED1_MODE, 0); // 0 = off

   Mat frame_now = Mat::zeros(rec_size, CV_8UC3);
   unsigned int fr = 0;
   struct timeval time0, time1;
   Mat frame_pre;
   bool do_record = false;
   bool take_diff = false;
   bool loop = true;
   while (loop) {
      gettimeofday(&time0, 0);
      Mat frame_raw;
      cap >> frame_raw;
      frame_raw(Rect(x0, y0, rec_size.width, rec_size.height)).copyTo(frame_now(Rect(0, 0, rec_size.width, rec_size.height)));

      Mat frame;
      if (! take_diff) {
	frame = frame_now.clone(); // = frame_now;
      } else {
         if (frame_pre.rows == 0) {
            frame_pre = frame_now.clone();
            continue;
         }
         absdiff(frame_pre, frame_now, frame);
         frame_pre = frame_now.clone();
      }

      time_t utime = time(0);
      tm* ltime = localtime(&utime);
      //string ts = asctime(ltime);
      char stime[256];
      strftime(stime, sizeof(stime),"%Y-%m-%d %H:%M:%S", ltime);
      char msg[256];
      sprintf(msg, "%s %06i", stime, fr);
      cv::putText (frame, msg, cv::Point(10, 20),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1.0,
                   CV_AA);
      
      if (do_record) {
         writer << frame;
         fr++;
      }
      if (scale != 1) resize(frame, frame, Size(), scale, scale);
      imshow("frame", frame);
      
      gettimeofday(&time1, NULL);
      int time_w = (int)round(1000.0/fps - 1e3*(time1.tv_sec - time0.tv_sec) - 1e-3*(time1.tv_usec - time0.tv_usec));
      if (time_w <= 0) {
         cout << "Wait time = " << time_w << " us.  FPS is too large." << endl;
         time_w = 1;
      }
      switch(waitKey(time_w)){
      case 'r':
         cout << "Start recording." << endl;
         do_record = true;
         break;
      case 's':
         cout << "Stop recording." << endl;
         do_record = false;
         break;
      case 'd':
         cout << "Set the frame-difference mode to '" << !take_diff << "'." << endl;
         take_diff = ! take_diff;
         break;
      case 'c':
         cout << "Control." << endl;
         oss.str("");
         oss << "guvcview --device=/dev/video" << dev_num << " --control_panel &";
         system(oss.str().c_str()); 
         break;
      case 'q':
         cout << "Quit." << endl;
         loop = false;
         break;
      default:
         break;
      }
   }

   writer.release();
   c_close_device(handle);
   c_cleanup();
   
   return 0;
}

void SetControl(CHandle handle, CControlId id, int value_new)
{
  CControlValue value;
  int ret = c_get_control(handle, id, &value);
  if (ret) {
    cout << "!!ERROR!!  c_get_control(): id=" << id << ", ret=" << ret << "\n";
    exit(1);
  }
  value.value = value_new;
  ret = c_set_control(handle, id, &value);
  if (ret) {
    cout << "!!ERROR!!  c_set_control(): id=" << id << ", ret=" << ret << "\n";
    exit(1);
  }
}

