#include <getopt.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <deque>
#include <ctime>
#include <sys/time.h>
#include <webcam.h>
using namespace cv;
using namespace std;
CDevice* GetDevInfo(CHandle handle);
void SetControl(CHandle handle, CControlId id, int value_new);
void ChangePlaybackPos(int& idx_pb, int val);
void SaveFrame(const string bname, const int idx_pb);

struct FrameData {
  int fr;
  Mat mat;
  FrameData(const int fr_in, const Mat* mat_in) {
    fr  = fr_in;
    mat = mat_in->clone();
  };
};
deque<FrameData> buffer_frame;

typedef enum { DISPLAY, RECORD, PLAYBACK } KirecoMode_t;
KirecoMode_t mode = DISPLAY;

int main(int argc, char** argv)
{
   int dev_num = 0;
   int fps = 15;
   int n_buf = fps * 10;
   int opt;
   while ((opt = getopt(argc, argv, "d:f:b:")) != -1) {
      switch (opt) {
      case 'd':
         dev_num = atoi(optarg);
         cout << "Device number (-d) = " << dev_num << "\n";
         break;
      case 'f':
         fps = atoi(optarg);
         cout << "FPS (-f) = " << fps << ".\n";
         break;
      case 'b':
         n_buf = atoi(optarg);
         cout << "N of buffered frames (-b) = " << n_buf << ".\n";
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
   char fn_base[256];
   strftime(fn_base, sizeof(fn_base),"kiribako-%Y-%m-%d--%H-%M-%S", ltime);

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
   CDevice *info = GetDevInfo(handle);
   string dev_name = info->name;
   cout << "Device name = " << dev_name << "\n";
   if (dev_name != "HD Webcam C615") {
     cout << "  Not 'HD Webcam C615'?  The device setting might be improper.\n";
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
   int idx_pb = 0;
   struct timeval time0, time1;
   Mat frame_pre;
   VideoWriter writer;   
   //bool do_record = false;
   bool take_diff = false;
   bool do_negate = false;
   bool loop      = true;
   while (loop) {
     gettimeofday(&time0, 0);

     if (mode == PLAYBACK) {
       cout << "Playback: " << idx_pb+1 << " / " << buffer_frame.size() << endl;
       Mat mat = buffer_frame.at(idx_pb).mat.clone();
       cv::putText(mat, "playback", cv::Point(cap_size.width-250, 50),
                   cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 0, 255), 4.0, CV_AA);
       imshow("frame", mat);
     } else { // DISPLAY or RECORD
       Mat frame_now;
       cap >> frame_now;
       i_fr++;
      
       Mat frame;
       if (! take_diff) {
         frame = frame_now;
       } else {
         if (frame_pre.rows == 0) { // frame_pre is not set
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
       cv::putText (frame, msg, cv::Point(50,50), cv::FONT_HERSHEY_SIMPLEX, 1.5,
                    cv::Scalar(100, 50, 50), 4.0, CV_AA);

       buffer_frame.push_back(FrameData(i_fr, &frame));
       if (buffer_frame.size() > (unsigned)n_buf) buffer_frame.pop_front();

       if (mode == RECORD) {
         if (! writer.isOpened()) {
           string fn_out = (string)fn_base + ".avi";
           if (! writer.open(fn_out.c_str(), CV_FOURCC('M','J','P','G'), fps, cap_size)) {
             cerr << "!!ERROR!!  Failed at opening the output file.  Abort.\n";
             exit(1);
           }
         }
         writer << frame;
         cv::putText(frame, "REC", cv::Point(cap_size.width-130, 50),
                     cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 0, 255), 4.0, CV_AA);
       }
       imshow("frame", frame);
     }
     
     int time_w;
     if (mode == PLAYBACK) time_w = 0;
     else {
       gettimeofday(&time1, NULL);
       time_w = (int)round(1000.0/fps - 1e3*(time1.tv_sec - time0.tv_sec) - 1e-3*(time1.tv_usec - time0.tv_usec));
       if (time_w <= 0) {
         cout << "Wait time = " << time_w << " us.  FPS is too large." << endl;
         time_w = 1;
       }
     }
     switch (waitKey(time_w) % 256) {
     case 'p': // Logicool C615 focus has 17 values, 0, 17, 34, 51, , , 238, 255
       c_get_control(handle, CC_FOCUS_ABSOLUTE, &value);
       if (value.value < 255) value.value += 17;
       c_set_control(handle, CC_FOCUS_ABSOLUTE, &value);
       ret = c_get_control(handle, CC_FOCUS_ABSOLUTE, &value);
       cout << "Focus = " << value.value << "\n";
       break;
     case 'o':
       c_get_control(handle, CC_FOCUS_ABSOLUTE, &value);
       if (value.value > 0) value.value -= 17;
       c_set_control(handle, CC_FOCUS_ABSOLUTE, &value);
       ret = c_get_control(handle, CC_FOCUS_ABSOLUTE, &value);
       cout << "Focus = " << value.value << "\n";
       break;
     case 'r':
       cout << "Start recording." << endl;
       mode = RECORD;
       break;
     case 's':
       cout << "Stop recording." << endl;
       mode = DISPLAY;
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
     case '.':   case 83: // left arrow
       ChangePlaybackPos(idx_pb, +1);
       break;
     case ',':   case 81: // right arrow
       ChangePlaybackPos(idx_pb, -1);
       break;
     case '>':   case 82: // up arrow
       ChangePlaybackPos(idx_pb, +fps);
       break;
     case '<':   case 84: // down arrow
       ChangePlaybackPos(idx_pb, -fps);
       break;
     case '/':
       mode = DISPLAY;
       break;
     case 'm':
       SaveFrame(fn_base, idx_pb);
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
  value.value = value_new;
  ret = c_set_control(handle, id, &value);
  if (ret) {
    cout << "!!ERROR!!  c_set_control(): id=" << id << ", ret=" << ret << "\n";
    exit(1);
  }
}

void ChangePlaybackPos(int& idx_pb, int val)
{
  int n_fr = buffer_frame.size();
  if (mode != PLAYBACK) {
    mode = PLAYBACK;
    idx_pb = n_fr;
  }
  int idx_new = idx_pb + val;
  if (idx_new < 0     ) idx_new = 0;
  if (idx_new > n_fr-1) idx_new = n_fr-1;
  idx_pb = idx_new;
}

void SaveFrame(const string bname, const int idx_pb)
{
  if (mode != PLAYBACK) return;
  FrameData* fd = &buffer_frame.at(idx_pb);
  ostringstream oss;
  oss << bname << "--" << setfill('0') << setw(6) << fd->fr << ".png";
  string fn_out = oss.str();
  cout << "  Save " << fn_out << endl;
  imwrite(fn_out, fd->mat);
}
