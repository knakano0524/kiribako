#include <getopt.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <sstream>
#include <ctime>
#include <sys/time.h>
#include <webcam.h>
using namespace cv;
using namespace std;

void SetControl(CHandle handle, CControlId id, int value_new);

int main(int argc, char** argv)
{
   int dev_num = 0;
   int fps = 15;
   int opt;
   while ((opt = getopt(argc, argv, "d:f:")) != -1) {
      switch (opt) {
      case 'd':
         dev_num = atoi(optarg);
         cout << "Device number (-d) = " << dev_num << "\n";
         break;
      case 'f':
         fps = atoi(optarg);
         cout << "FPS (-f) = " << fps << ".\n";
         break;
      default:
         cerr << "Invalid option.  Abort.\n";
         exit(1);
      }
   }
   
   CResult ret;
   CHandle handle;
   CControlValue value;
   
   VideoCapture cap(dev_num);
   if(!cap.isOpened()){
      return -1;
   }
   Size cap_size (1280, 720);
   cap.set (CV_CAP_PROP_FPS         , fps            );
   cap.set (CV_CAP_PROP_FRAME_WIDTH , cap_size.width );
   cap.set (CV_CAP_PROP_FRAME_HEIGHT, cap_size.height);
   
   time_t utime = time(0);
   tm* ltime = localtime(&utime);
   char fn_out[256];
   strftime(fn_out, sizeof(fn_out),"kiribako-%Y-%m-%d--%H-%M-%S.avi", ltime);
   VideoWriter writer;   
   
   //initialize webcam
   ret = c_init();
   if (ret) {
      cout << "Error : c_init() : " << ret << "\n";
      return 1;
   }
   
   ostringstream oss;
   oss << "video" << dev_num;
   handle = c_open_device(oss.str().c_str());
   if(!handle){
      cout << "Error : c_open_device() : " << ret << "\n";
      c_cleanup();
      return 1;
   }

   SetControl(handle, CC_BRIGHTNESS    , 120);
   SetControl(handle, CC_CONTRAST      ,  50);
   SetControl(handle, CC_SATURATION    ,  20);
   SetControl(handle, CC_SHARPNESS     ,  50);
   SetControl(handle, CC_AUTO_FOCUS    ,   0);
   SetControl(handle, CC_FOCUS_ABSOLUTE, 119); // 119 for zoom=1, 85 for zoom=2
   SetControl(handle, CC_ZOOM_ABSOLUTE ,   1); // 1 or 2
   SetControl(handle, CC_POWER_LINE_FREQUENCY, 1); // 0 = off, 1 = 50 Hz, 2 = 60 Hz
   SetControl(handle, CC_LOGITECH_LED1_MODE, 0); // 0 = off
   namedWindow("frame");

   unsigned int i_fr = 0;
   struct timeval time0, time1;
   Mat frame_pre;
   bool do_record = false;
   bool take_diff = false;
   bool do_negate = false;
   bool loop      = true;
   while (loop) {
      gettimeofday(&time0, 0);
      Mat frame_now;
      cap >> frame_now;
      //cvtColor(frame, edges, CV_BGR2GRAY);
      //GaussianBlur(edges, edges, Size(7,7), 1.5, 1.5);
      //Canny(edges, edges, 0, 30, 3);
      
      Mat frame;
      if (! take_diff) {
         frame = frame_now;
      } else {
         if (frame_pre.rows == 0) {
            frame_pre = frame_now;
            continue;
         }
         absdiff(frame_pre, frame_now, frame);
         frame_pre = frame_now;
      }
      if (do_negate) {
         static Mat mat0 = Mat(frame.rows, frame.cols, frame.type(), Scalar(1,1,1))*255;
         subtract(mat0, frame, frame);
      }
      
      time_t utime = time(0);
      tm* ltime = localtime(&utime);
      char stime[256];
      strftime(stime, sizeof(stime),"%Y-%m-%d %H:%M:%S", ltime);
      char msg[256];
      sprintf(msg, "%s %06i", stime, i_fr);
      cv::putText (frame, msg, cv::Point (50,50), cv::FONT_HERSHEY_SIMPLEX, 2,
                   cv::Scalar (100, 50, 50), 5, CV_AA);
      if (do_record) {
         if (i_fr == 0) writer.open(fn_out, CV_FOURCC('M','J','P','G'), fps, cap_size);
         writer << frame;
         i_fr++;
         cv::putText(frame, "REC", cv::Point(cap_size.width - 130, 50),
                     cv::FONT_HERSHEY_SIMPLEX, 2,
                     cv::Scalar(0, 0, 255), 5, CV_AA);
      }
      imshow("frame", frame);
      
      gettimeofday(&time1, NULL);
      int time_w = (int)round(1000.0/fps - 1e3*(time1.tv_sec - time0.tv_sec) - 1e-3*(time1.tv_usec - time0.tv_usec));
      if (time_w <= 0) {
         cout << "Wait time = " << time_w << " us.  FPS is too large." << endl;
         time_w = 1;
      }
      switch(waitKey(time_w)){
         //Logicool C615 focus has 17 values, 0, 17, 34, 51, , , 238, 255
      case 'p':
         c_get_control(handle, CC_FOCUS_ABSOLUTE, &value);
         if (value.value < 255) value.value += 17;
         c_set_control(handle, CC_FOCUS_ABSOLUTE, &value);
         ret = c_get_control(handle, CC_FOCUS_ABSOLUTE, &value);
         cout << "Focus = " << value.value << "\n";
         break;
      case 'm':
         c_get_control(handle, CC_FOCUS_ABSOLUTE, &value);
         if (value.value > 0) value.value -= 17;
         c_set_control(handle, CC_FOCUS_ABSOLUTE, &value);
         ret = c_get_control(handle, CC_FOCUS_ABSOLUTE, &value);
         cout << "Focus = " << value.value << "\n";
         break;
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
      case 'n':
         cout << "Set the negative mode to '" << !do_negate << "'." << endl;
         do_negate = ! do_negate;
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

