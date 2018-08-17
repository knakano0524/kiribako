#include <getopt.h>
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
const int scale = 1; // Resizing factor.  Applied for only display, not recording.
const string win_name = "main";
Point sel1 = Point(-1, -1), sel2 = Point(-1, -1); /// region selected
CDevice* GetDevInfo(CHandle handle);
void SetControl(CHandle handle, CControlId id, int value_new);
void MouseHandler(int event, int x, int y, int flags, void* param);
void DrawRect(Mat *fr);
  
////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
   int dev_num = 0;
   //int fps = 15; // Others
   //int fps = 8; // Coolingtech Digital Microscope
   int fps =  10; // Dino-Lite
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
   
   time_t utime = time(0);
   tm* ltime = localtime(&utime);
   char fn_out[256];
   strftime(fn_out, sizeof(fn_out),"iontrap-%Y-%m-%d--%H-%M-%S.avi", ltime);
   VideoWriter writer;

   CResult ret = c_init();
   if (ret) {
      cout << "Error : c_init() : " << ret << "\n";
      return 1;
   }
   
   ostringstream oss;
   oss << "video" << dev_num;
   CHandle handle = c_open_device(oss.str().c_str());
   if (!handle) {
      cout << "Error : c_open_device()\n";
      c_cleanup();
      return 1;
   }
   CDevice *info = GetDevInfo(handle);
   string dev_name = info->name;
   cout << "Device name = " << dev_name << "\n";

   VideoCapture cap(dev_num);
   if (!cap.isOpened()) {
     cerr << "!!ERROR!!  Cannot open 'cap'.  Abort.\n";
     return 1;
   }
   Size cap_size;
   Size rec_size; //< Record only this center part of cap_size.
   if (dev_name == "USB camera") { // Dino-Lite 311
     // can run with >10 fps.
     // bri 127, con 127, gam 40, viv 255
     fps = 10;
     cap_size.width  = 640;
     cap_size.height = 480;
     rec_size.width  = cap_size.height; // HxH
     rec_size.height = cap_size.height; // HxH
   } else if (dev_name == "Dino-Lite Basic") { // Dino-Lite 2111
     // bri -48, con 64, gam 109, gain 30, backlight 0
     // 15 fps when the gauge is inserted, but 5 fps when it isn't.
     // Is it because the devices tries an auto exposure adjustment??
     fps = 10;
     cap_size.width  = 640;
     cap_size.height = 480;
     rec_size.width  = cap_size.height; // HxH
     rec_size.height = cap_size.height; // HxH
   } else if (dev_name == "???") { // Teslong
     // 15 fps
     fps = 15;
     cap_size.width  = 640;
     cap_size.height = 480;
     rec_size.width  = cap_size.height; // HxH
     rec_size.height = cap_size.height; // HxH
   } else if (dev_name == "USB2.0 UVC PC Camera") { // Coolingtech DM
     fps = 9; // Max seems 8 regardless of settings.  Not good.
     cap_size.width  = 640; // max = 640
     cap_size.height = 480; // max = 480
     rec_size.width  = cap_size.height; // HxH
     rec_size.height = cap_size.height; // HxH
   } else { // Default
     cout << "Use the default setting.  probably not optimal.\n";
     fps = 15;
     cap_size.width  = 640;
     cap_size.height = 480;
     rec_size.width  = cap_size.height; // HxH
     rec_size.height = cap_size.height; // HxH
     //SetControl(handle, CC_BRIGHTNESS    , 120);
     //SetControl(handle, CC_CONTRAST      ,  50);
     //SetControl(handle, CC_SATURATION    ,  20);
     //SetControl(handle, CC_SHARPNESS     ,  50);
     //SetControl(handle, CC_AUTO_EXPOSURE_MODE    ,   1); // 1=off, 3=on
     //SetControl(handle, CC_EXPOSURE_TIME_ABSOLUTE, 600); // depends on FPS
     //SetControl(handle, CC_AUTO_FOCUS    ,   0);
     //SetControl(handle, CC_FOCUS_ABSOLUTE, 255);
     //SetControl(handle, CC_ZOOM_ABSOLUTE ,   2);
     //SetControl(handle, CC_POWER_LINE_FREQUENCY, 1); // 1 = 50 Hz
   }

   cap.set(CV_CAP_PROP_FPS         , fps            );
   cap.set(CV_CAP_PROP_FRAME_WIDTH , cap_size.width );
   cap.set(CV_CAP_PROP_FRAME_HEIGHT, cap_size.height);
   int x0 = (cap_size.width  - rec_size.width ) / 2;
   int y0 = (cap_size.height - rec_size.height) / 2;
   Mat frame_now = Mat::zeros(rec_size, CV_8UC3);
   namedWindow(win_name);
   setMouseCallback(win_name, MouseHandler, 0);
   
   unsigned int i_fr = 0;
   struct timeval time0, time1;
   bool do_record = false;
   bool loop = true;
   while (loop) {
      gettimeofday(&time0, 0);
      Mat frame_raw;
      cap >> frame_raw;
      frame_raw(Rect(x0, y0, rec_size.width, rec_size.height)).copyTo(frame_now(Rect(0, 0, rec_size.width, rec_size.height)));
      Mat frame = frame_now; // no modification

      time_t utime = time(0);
      tm* ltime = localtime(&utime);
      char stime[256];
      strftime(stime, sizeof(stime),"%Y-%m-%d %H:%M:%S", ltime);
      char msg[256];
      sprintf(msg, "%s %06i", stime, i_fr);
      cv::putText(frame, msg, cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX,
                  0.5, cv::Scalar(0, 255, 0), 1.0, CV_AA);
      if (do_record) {
         if (i_fr == 0) writer.open(fn_out, CV_FOURCC('M','J','P','G'), fps, rec_size);
         writer << frame;
         i_fr++;
         cv::putText(frame, "REC", cv::Point(rec_size.width - 40, 20),
                     cv::FONT_HERSHEY_SIMPLEX, 0.5,
                     cv::Scalar(0, 0, 255), 1.0, CV_AA);
      }
      if (scale != 1) resize(frame, frame, Size(), scale, scale);
      DrawRect(&frame);
      imshow(win_name, frame);

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

CDevice* GetDevInfo(CHandle handle)
{
  unsigned int size = sizeof(CDevice) + 32 + 84;
  CDevice *info = (CDevice*)malloc(size);
  int ret = c_get_device_info(handle, 0, info, &size);
  if (ret != 0) {
    cerr << "!!ERROR!!  GetDevInfo(): c_get_device_info() returned " << ret
	 << ".  Abort.\n";
    exit(1);
  }
  //cout << "GetDevInfo():"
  //     << "\n  shortname = " << info->shortName
  //     << "\n       name = " << info->name
  //     << "\n     driver = " << info->driver
  //     << "\n   location = " << info->location
  //     << endl;
  return info;
}

void SetControl(CHandle handle, CControlId id, int value_new)
{
  CControlValue value;
  int ret = c_get_control(handle, id, &value);
  if (ret) {
    cout << "!!ERROR!!  c_get_control(): id=" << id << ", ret=" << ret << "\n";
    exit(1);
  }
  //cout << "SetControl(): id=" << id << ": " << value.value << " -> " << value_new << endl;
  if (value.value == value_new) return;
  value.value = value_new;
  ret = c_set_control(handle, id, &value);
  if (ret) {
    cout << "!!ERROR!!  c_set_control(): id=" << id << ", ret=" << ret << "\n";
    exit(1);
  }
}

void MouseHandler(int event, int x, int y, int flags, void* param)
{
   if (event == CV_EVENT_LBUTTONDOWN) {
      sel1 = cv::Point(x, y);
      //req_draw = true;
   } else if (event == CV_EVENT_RBUTTONDOWN) {
      sel2 = cv::Point(x, y);
      //req_draw = true;
   } else if (event == CV_EVENT_MBUTTONDOWN) {
      sel1 = cv::Point(-1, -1);
      sel2 = cv::Point(-1, -1);
      //req_draw = true;
   }
}

void DrawRect(Mat *fr)
{
   ostringstream oss;
   if (sel1.x >= 0) {
      oss << "L: (" << sel1.x << ", " << sel1.y << ")";
      cv::putText (*fr, oss.str(), cv::Point(sel1.x, sel1.y-5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1.0, CV_AA);
      circle(*fr, sel1, 3, CV_RGB(255, 0, 0), -1);
   }
   if (sel2.x >= 0) {
      oss.str("");
      oss << "R: (" << sel2.x << ", " << sel2.y << ")";
      cv::putText (*fr, oss.str(), cv::Point(sel2.x+5, sel2.y), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1.0, CV_AA);
      circle(*fr, sel2, 3, CV_RGB(255, 0, 0), -1);
   }
   if (sel1.x >= 0 && sel2.x >= 0) {
      oss.str("");
      oss << abs(sel2.x-sel1.x) << "x" << abs(sel2.y-sel1.y);
      cv::putText (*fr, oss.str(), cv::Point(sel2.x+5, (sel1.y+sel2.y)/2), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1.0, CV_AA);
      rectangle(*fr, sel1, sel2, CV_RGB(255, 0, 0));
   }
}
