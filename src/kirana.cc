#include <dirent.h>
#include <sys/stat.h>
#include <iostream>
#include <iomanip>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace cv;
using namespace std;
const string win_name = "main";
string base_name;
string name_trk;
VideoCapture cap;
Point point1, point2 = Point(-1, -1);
int n_fr;
int i_fr = 0;
Mat frame;
bool paused = false;
int JumpFrame(int fr);
int JumpFrameRel(int move);
string FindLastFile();
void SaveFrame();
void MouseHandler(int event, int x, int y, int flags, void* param);
void PosTrackerHandler(int val, void* param);

////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
   string fname = "demo_beta.mp4";
   if (argc >= 2) fname = argv[1];
   else           fname = FindLastFile();
   struct stat st;
   if (stat(fname.c_str(), &st) != 0) {
      cerr << "Cannot open the input file '" << fname << "'.  Abort.\n";
      exit(1);
   }

   cap.open(fname);
   double fps  = cap.get(CV_CAP_PROP_FPS);
   int msec_fr = (int)(1000 / fps);
   n_fr        = (int)(cap.get(CV_CAP_PROP_FRAME_COUNT));
   cout << "Input: " << fname << "\n"
        << "  FPS            = " << fps << "\n"
        << "  Frame interval = " << msec_fr << " ms\n"
        << "  N of frames    = " << n_fr << "\n\n";

   namedWindow(win_name);
   //namedWindow("line" );
   setMouseCallback(win_name, MouseHandler, 0);
   ostringstream oss;
   oss << "Frame (/" << n_fr << ")";
   name_trk = oss.str();
   createTrackbar(name_trk, win_name, 0, n_fr-1, PosTrackerHandler);

   int idx_head = fname.find_last_of('/') + 1;
   base_name = fname.substr(idx_head, fname.find_last_of('.') - idx_head);
   
   Mat frame_pre;
   int i_fr_pre = -1;
   bool auto_diff = false;
   bool do_loop = true;
   while (do_loop) {
      if (i_fr != i_fr_pre) {
         cout << "frame " << i_fr << " / " << n_fr << endl;
         cap >> frame;

         //Mat out = Mat::zeros(frame.size(), frame.type());
         //int cols = frame.cols;
         //int rows = frame.rows;
         //int pix  = i_fr * 2;
         //frame(Rect(0, 0, cols - pix, rows)).copyTo(out(Rect(pix, 0, cols - pix, rows)));
         ////frame = out.clone();
         //addWeighted(frame, 1.0, out, 1.0, 0.0, frame);
         
         if (auto_diff) {
            double mean = cv::mean(frame)[0];
            //cout << "  " << i_fr << " " << mean << endl;
            if (mean > 20) { // take a difference
               Mat frame_tmp = frame.clone();
               absdiff(frame_pre, frame_tmp, frame);
               frame_pre = frame_tmp.clone();
            } else {
               frame_pre = frame.clone();
            }
         }
         if (point2.x >= 0) {
            rectangle(frame, point1, point2, CV_RGB(255, 0, 0), 1, 8, 0);
         }
         imshow(win_name, frame);
         setTrackbarPos(name_trk, win_name, i_fr);
         i_fr_pre = i_fr;
      }

      //Mat dst, cdst;
      //Canny(frame, dst, 50, 200, 3);
      //cvtColor(dst, cdst, CV_GRAY2BGR);
      //vector<Vec4i> lines;
      //const double reso_rho   = 5;
      //const double reso_theta = CV_PI/180;
      //const int    accu_thre  =  50; // 50
      //const double min_length =  50; // 50
      //const double max_gap    =  10; // 10
      //HoughLinesP(dst, lines, reso_rho, reso_theta, accu_thre, min_length, max_gap);
      //for (size_t i = 0; i < lines.size(); i++) {
      //   Vec4i l = lines[i];
      //   line( cdst, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
      //}
      //imshow("line", cdst);
      //waitKey(0);
       
      //if (waitKey(msec_fr) >= 0) break;
      switch (waitKey(msec_fr)) {
      case ' ':
         paused = ! paused;
         cout << "Pause = " << paused << endl;
         break;
      case 's':
         if (paused) SaveFrame();
         else cout << "Set paused to save the frame." << endl;
         break;
      case 'd':
         auto_diff = ! auto_diff;
         cout << "Auto-differential mode = " << auto_diff << endl;
         break;
      case '.':
         paused = true;
         i_fr = JumpFrameRel(+1);
         break;
      case ',':
         paused = true;
         i_fr = JumpFrameRel(-1);
         break;
      case 'n':
         i_fr = JumpFrameRel(+fps);
         break;
      case 'p':
         i_fr = JumpFrameRel(-fps);
         break;
      case 'q':
         cout << "Quit." << endl;
         do_loop = false;
         break;
      default:
         if (! paused && i_fr < n_fr - 1) i_fr++;
         break;
      }         
   }
   
   cout << "END" << endl;
   return 0;
}

////////////////////////////////////////////////////////////////
int JumpFrame(int fr)
{
   if (0 <= fr && fr < n_fr) {
      i_fr = fr;
      cap.set(CV_CAP_PROP_POS_FRAMES, i_fr);
   }
   return i_fr;
}

int JumpFrameRel(int move)
{
   return JumpFrame(i_fr + move);
}

string FindLastFile()
{
   struct dirent **namelist;
   int n_fi = scandir(".", &namelist, 0, alphasort);
   vector<string> list;
   for (int i_fi = n_fi-1; i_fi >= 0; i_fi--) {
      string fname = namelist[i_fi]->d_name;
      //cout << fname << endl;
      if (fname.substr(0, 9) == "kiribako-") return fname;
   }
   return "";
}

void SaveFrame()
{
   ostringstream oss;
   oss << "frame_" << base_name << "_" << setfill('0') << setw(4) << i_fr << ".png";
   string fn_out = oss.str();
   cout << "  Save " << fn_out << endl;
   imwrite(fn_out, frame);
}

void MouseHandler(int event, int x, int y, int flags, void* param)
{
   //cout << "MouseHandler: " << event << " " << x << " " << y << " " << flags << " " << CV_EVENT_FLAG_LBUTTON << endl;
   if (event == CV_EVENT_RBUTTONUP) { // Reset the region
      point2 = cv::Point(-1, -1);
   }
   if (event == CV_EVENT_LBUTTONDOWN) { // Start the region
      point1 = cv::Point(x, y);
      point2 = cv::Point(-1, -1);
   }
   if ((flags & CV_EVENT_LBUTTONDOWN)) {
      if (event == CV_EVENT_MOUSEMOVE) {
         //cv::Mat img1 = frame.clone();
         point2 = cv::Point(x, y);
         //rectangle(frame, point1, point2, CV_RGB(255, 0, 0), 3, 8, 0);
         //imshow(win_name, frame);
      }
      if (event == CV_EVENT_LBUTTONUP) {
         //cv::Mat img2 = frame.clone();
         point2 = cv::Point(x, y);
         //rectangle(frame, point1, point2, CV_RGB(255, 0, 0), 3, 8, 0);
         //imshow(win_name, frame);
      }
   }
}

void PosTrackerHandler(int val, void* param)
{
   if (val != i_fr) {
      paused = true;
      JumpFrame(val);
   }
}
