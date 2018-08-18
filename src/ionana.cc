#include <getopt.h>
#include <dirent.h>
#include <sys/stat.h>
#include <iostream>
#include <iomanip>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace cv;
using namespace std;
const int scale = 2; // resizing factor
const string win_name = "main";
int verb = 0;
string base_name;
string name_trk;
VideoCapture cap;
Point sel1 = Point(-1, -1), sel2 = Point(-1, -1); /// region selected
Point cut1 = Point(-1, -1), cut2 = Point(-1, -1); /// region to be cut out
int n_fr;
int i_fr = 0;
int fr_1 = -1, fr_2 = -1; /// range for composition
int pix_slide = 2; // pixels per frame slided in the composition mode
Mat frame_draw;
bool paused = false;
bool req_draw = false;
bool sum_mode  = false;
vector<int> SplitArg(const string arg, const char delim=':');
string FindLastFile();
//string ReadGroupLabel();
int JumpFrame(int fr);
int JumpFrameRel(int move);
void SaveFrame();
void MouseHandler(int event, int x, int y, int flags, void* param);
void PosTrackerHandler(int val, void* param);
void DrawRect(Mat *fr);
void TakeDiffAuto(Mat* frame, bool force_init=false);
void SumFramesV1(Mat* fr, int fr_1, int fr_2);
void SumFrames(Mat* fr, int fr_1, int fr_2);

////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
   string fname = "";
   int opt;
   vector<int> values;
   while ((opt = getopt(argc, argv, "vl:i:f:c:")) != -1) {
      switch (opt) {
      case 'v':
         verb = 1;
         break;
      case 'l':
         pix_slide = atoi(optarg);
         break;
      case 'i':
         fname = optarg;
         cout << "Input file (-i) = " << fname << ".\n";
         break;
      case 'f':
         values = SplitArg(optarg, ':');
         if (values.size() != 2) {
            cerr << "!!ERROR!!  Option '-f' requires two integers.\n";
            exit(1);
         }
         fr_1 = values[0];
         fr_2 = values[1];
         cout << "Frame range for composition (-f) = " << fr_1 << "..." << fr_2 << ".\n";
         break;
      case 'c':
         values = SplitArg(optarg, ':');
         if (values.size() == 2) {
            cut1 = Point(values[0], -1);
            cut2 = Point(values[1], -1);
         } else if (values.size() == 4) {
            cut1 = Point(values[0], values[2]);
            cut2 = Point(values[1], values[3]);
         } else {
            cerr << "!!ERROR!!  Option '-c' requires two or four integers.\n";
            exit(1);
         }
         cout << "Pixel range for compsition (-c) = " << cut1 << "..." << cut2 << ".\n";
         break;
      default:
         cerr << "Invalid option.  Abort.\n";
         exit(1);
      }
   }
 
   if (fname.length() == 0) fname = FindLastFile();
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

   if (cut1.x >= 0) { // When the cut region is given by option "-c"
      if (cut1.y < 0) {
         cut1.y = 0;
         cut2.y = 2 * (int)(cap.get(CV_CAP_PROP_FRAME_HEIGHT)) - 1;
      }
      sel1 = cut1;
      sel2 = cut2;
   }
   
   namedWindow(win_name);
   setMouseCallback(win_name, MouseHandler, 0);
   ostringstream oss;
   oss << "Frame (/" << n_fr << ")";
   name_trk = oss.str();
   createTrackbar(name_trk, win_name, 0, n_fr-1, PosTrackerHandler);

   int idx_head = fname.find_last_of('/') + 1;
   base_name = fname.substr(idx_head, fname.find_last_of('.') - idx_head);

   Mat frame_cap;
   Mat frame_sum;
   int i_fr_pre = -1;
   bool auto_diff = false;
   bool do_loop   = true;
   sum_mode  = false;
   while (do_loop) {
      //if (fr_2 >= 0) { // In the sum mode
      //   frame = frame_sum.clone();
      //   //DrawRect(&frame);
      //   //imshow(win_name, frame);
      //   req_draw = true;
      //} else
      if (i_fr != i_fr_pre) {
         if (verb > 0) cout << "frame " << i_fr << " / " << n_fr << endl;
         cap >> frame_cap;
         resize(frame_cap, frame_cap, Size(), scale, scale);
         if (auto_diff) TakeDiffAuto(&frame_cap);
         i_fr_pre = i_fr;
         req_draw = true;
      }
      if (req_draw) {
         if (sum_mode) frame_draw = frame_sum.clone();
         else          frame_draw = frame_cap.clone();
         DrawRect(&frame_draw);
         imshow(win_name, frame_draw);
         setTrackbarPos(name_trk, win_name, i_fr);
         req_draw = false;
      }
      
      switch (waitKey(msec_fr)) {
      case ' ':
         paused = ! paused;
         cout << "Pause = " << paused << endl;
         break;
      case 's':
	 SaveFrame();
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
      case '1':
         cout << "Start frame for composition = " << i_fr << ".\n";
         fr_1 = i_fr;
         break;
      case '2':
         cout << "End frame for composition = " << i_fr << ".\n";
         fr_2 = i_fr;
         break;
      case '3':
         cout << "Enter the composition mode.\n";
         if (fr_1 < 0 || fr_2 < 0) {
            cout << "  Actually not possible.  Set both the start & end frames.\n";
         } else {
            req_draw = true;
            sum_mode = true;
            SumFrames(&frame_sum, fr_1, fr_2);
            i_fr_pre = i_fr = fr_2;
         }
         break;
      case '4':
         cout << "Exit the composition mode.\n";
         sum_mode = false;
         //fr_1 = fr_2 = -1;
         break;
      case 'c':
         if (sel1.x < 0) {
            cout << "Cannot set the cut region.  Select a region first." << endl;
         } else {
            cout << "Set the cut region." << endl;
            cut1 = sel1;
            cut2 = sel2;
         }
         break;
      case 'x':
         cout << "Reset the cut region." << endl;
         cut1 = Point(-1, -1);
         cut2 = Point(-1, -1);
         break;
      case 'q':
         cout << "Quit." << endl;
         do_loop = false;
         break;
      default:
         if (! paused && ! sum_mode && i_fr < n_fr - 1) i_fr++;
         break;
      }
   }
   
   cout << "* * E N D * *" << endl;
   return 0;
}

////////////////////////////////////////////////////////////////
vector<int> SplitArg(const string arg, const char delim)
{
   string arg2 = arg;
   replace(arg2.begin(), arg2.end(), delim, ' ');
   istringstream iss(arg2);
   int val;
   vector<int> values;
   while (iss >> val) values.push_back(val);
   return values;
}

string FindLastFile()
{
   struct dirent **namelist;
   int n_fi = scandir(".", &namelist, 0, alphasort);
   vector<string> list;
   for (int i_fi = n_fi-1; i_fi >= 0; i_fi--) {
      string fname = namelist[i_fi]->d_name;
      //cout << fname << endl;
      if (fname.substr(0, 8) == "iontrap-") return fname;
   }
   return "";
}

//string ReadGroupLabel()
//{
//   ostringstream oss;
//   oss << getenv("HOME") << "/.b3exp.conf";
//   ifstream ifs(oss.str().c_str());
//   if (ifs.is_open()) return "";
//   string label;
//   ifs >> label;
//   ifs.close();
//   return label;
//}

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

void SaveFrame()
{
   if (!paused && !sum_mode) {
     cout << "Need to set 'paused' or go into the composition mode to save the frame." << endl;
     return;
   }
   ostringstream oss;
   oss << "frame_" << base_name << "_" << setfill('0');
   if (sum_mode) oss << setw(6) << fr_1 << "-" << setw(6) << fr_2 << ".png";
   else          oss << setw(6) << i_fr << ".png";
   string fn_out = oss.str();
   cout << "  Save " << fn_out << endl;
   imwrite(fn_out, frame_draw);
}

void MouseHandler(int event, int x, int y, int flags, void* param)
{
   //cout << "MouseHandler: " << event << " " << x << " " << y << " " << flags
   //     << " " << CV_EVENT_FLAG_LBUTTON << endl;
   if (event == CV_EVENT_LBUTTONDOWN) {
      sel1 = cv::Point(x, y);
      req_draw = true;
      //if (sel2.x < 0) sel2 = cv::Point(x, y);
   } else if (event == CV_EVENT_RBUTTONDOWN) {
      sel2 = cv::Point(x, y);
      req_draw = true;
      //if (sel1.x < 0) sel1 = cv::Point(x, y);
   } else if (event == CV_EVENT_MBUTTONDOWN) {
      sel1 = cv::Point(-1, -1);
      sel2 = cv::Point(-1, -1);
      req_draw = true;
   }

   //if (event == CV_EVENT_RBUTTONUP) { // Reset the region
   //   sel2 = cv::Point(-1, -1);
   //}
   //if (event == CV_EVENT_LBUTTONDOWN) { // Start the region
   //   sel1 = cv::Point(x, y);
   //   sel2 = cv::Point(-1, -1);
   //}
   //if ((flags & CV_EVENT_LBUTTONDOWN)) {
   //   if (event == CV_EVENT_MOUSEMOVE) {
   //      //cv::Mat img1 = frame.clone();
   //      sel2 = cv::Point(x, y);
   //      //rectangle(frame, sel1, sel2, CV_RGB(255, 0, 0), 3, 8, 0);
   //      //imshow(win_name, frame);
   //   }
   //   if (event == CV_EVENT_LBUTTONUP) {
   //      //cv::Mat img2 = frame.clone();
   //      sel2 = cv::Point(x, y);
   //      //rectangle(frame, sel1, sel2, CV_RGB(255, 0, 0), 3, 8, 0);
   //      //imshow(win_name, frame);
   //   }
   //}
}

void PosTrackerHandler(int val, void* param)
{
   if (val != i_fr) {
      //paused = true;
      JumpFrame(val);
   }
}

void DrawRect(Mat *fr)
{
   //if (sel1.x < 0 || sel2.x < 0) return;

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

void TakeDiffAuto(Mat* frame, bool force_init)
{
   static Mat frame_pre = Mat::zeros(Size(0, 0), CV_8U);
   if (force_init || frame_pre.rows == 0) {
      frame_pre = frame->clone();
      return;
   }
   double mean = cv::mean(*frame)[0];
   if (mean > 20) { // take a difference
      Mat frame_tmp = frame->clone();
      //absdiff(frame_pre, frame_tmp, *frame);
      *frame = frame_tmp - frame_pre;
      frame_pre = frame_tmp.clone();
   } else {
      frame_pre = frame->clone();
   }
}

/** Version 1
 * Background = previous frame.
 */
void SumFramesV1(Mat* fr, int fr_1, int fr_2)
{
   const bool use_gray = true; // faster??  not checked
   cout << "Composing frames " << fr_1 << "..." << fr_2 << " (V1)\n";
   JumpFrame(fr_1);
   Mat frame_raw;
   cap >> frame_raw;
   //Canny(frame_raw, frame_raw, 50, 200, 3);
   resize(frame_raw, frame_raw, Size(), scale, scale);
   if (use_gray) cvtColor(frame_raw, frame_raw, CV_BGR2GRAY);

   int xx2, yy2, dx2, dy2; // cut region
   if (cut1.x >= 0) {
      xx2 = cut1.x < cut2.x ? cut1.x : cut2.x;
      yy2 = cut1.y < cut2.y ? cut1.y : cut2.y;
      dx2 = abs(cut1.x - cut2.x);
      dy2 = abs(cut1.y - cut2.y);
      //// Reset the selection since the window size will change
      sel1 = cv::Point(-1, -1);
      sel2 = cv::Point(-1, -1);
   } else {
      xx2 = yy2 = 0;
      dx2 = frame_raw.cols;
      dy2 = frame_raw.rows;
   }
   *fr = Mat::zeros(Size(dx2 + 100 + pix_slide*(fr_2-fr_1), dy2), frame_raw.type());
   TakeDiffAuto(&frame_raw, true);

   for (int i_fr = fr_1 + 1; i_fr <= fr_2; i_fr++) {
      cap >> frame_raw;
      //Canny(frame_raw, frame_raw, 50, 200, 3);
      resize(frame_raw, frame_raw, Size(), scale, scale);
      if (use_gray) cvtColor(frame_raw, frame_raw, CV_BGR2GRAY);
      TakeDiffAuto(&frame_raw);
      
      Mat frame_mod = Mat::zeros(fr->size(), fr->type());
      int sx = pix_slide * (i_fr - fr_1);
      frame_raw(Rect(xx2, yy2, dx2, dy2)).copyTo(frame_mod(Rect(sx, 0, dx2, dy2)));
      addWeighted(frame_mod, 1.0, *fr, 1.0, 0.0, *fr);
   }
   if (use_gray) cvtColor(*fr, *fr, CV_GRAY2BGR);
      //// old version
      //int sx  = pix_slide * (i_fr - (fr_1 + fr_2) / 2);
      //if (sx > 0) frame_raw(Rect(  0, 0, xx-sx, yy)).copyTo(frame_mod(Rect(sx, 0, xx-sx, yy)));
      //else        frame_raw(Rect(-sx, 0, xx+sx, yy)).copyTo(frame_mod(Rect( 0, 0, xx+sx, yy)));
      //if (cut1.x >= 0) {
      //   frame_raw(Rect(xx2, yy2, dx2, dy2)).copyTo(frame_mod(Rect(xx2+sx, yy2, dx2, dy2)));
      //} else {
      //   if (sx > 0) frame_raw(Rect(  0, 0, xx-sx, yy)).copyTo(frame_mod(Rect(sx, 0, xx-sx, yy)));
      //   else        frame_raw(Rect(-sx, 0, xx+sx, yy)).copyTo(frame_mod(Rect( 0, 0, xx+sx, yy)));
      //}
}

/** Version 2
 * Background = average of all frames selected.
 */
void SumFrames(Mat* fr, int fr_1, int fr_2)
{
   cout << "Composing frames " << fr_1 << "..." << fr_2 << endl;
   Mat frame_raw;

   ///
   /// Estimate the background
   ///
   JumpFrame(fr_1);
   cap >> frame_raw;
   frame_raw.convertTo(frame_raw, CV_32FC3);
   Mat frame_bg = Mat::zeros(frame_raw.size(), frame_raw.type());
   frame_bg += frame_raw;
   for (int i_fr = fr_1 + 1; i_fr <= fr_2; i_fr++) {
     cap >> frame_raw;
     frame_raw.convertTo(frame_raw, frame_bg.type());
     frame_bg += frame_raw;
   }
   frame_bg /= fr_2-fr_1+1;
   frame_bg.convertTo(frame_bg, CV_8UC3);
   //imshow(win_name, frame_bg);  waitKey(0);
   
   ///
   /// Sum up all frames with background subtraction
   ///
   JumpFrame(fr_1);
   cap >> frame_raw;
   frame_raw -= frame_bg;
   resize(frame_raw, frame_raw, Size(), scale, scale);

   int xx2, yy2, dx2, dy2; // cut region
   if (cut1.x >= 0) {
      xx2 = cut1.x < cut2.x ? cut1.x : cut2.x;
      yy2 = cut1.y < cut2.y ? cut1.y : cut2.y;
      dx2 = abs(cut1.x - cut2.x);
      dy2 = abs(cut1.y - cut2.y);
      //// Reset the selection since the window size will change
      sel1 = cv::Point(-1, -1);
      sel2 = cv::Point(-1, -1);
   } else {
      xx2 = yy2 = 0;
      dx2 = frame_raw.cols;
      dy2 = frame_raw.rows;
   }
   *fr = Mat::zeros(Size(dx2 + 100 + pix_slide*(fr_2-fr_1), dy2), frame_raw.type());

   for (int i_fr = fr_1 + 1; i_fr <= fr_2; i_fr++) {
      cap >> frame_raw;
      frame_raw -= frame_bg;
      resize(frame_raw, frame_raw, Size(), scale, scale);
      Mat frame_mod = Mat::zeros(fr->size(), fr->type());
      int sx = pix_slide * (i_fr - fr_1);
      frame_raw(Rect(xx2, yy2, dx2, dy2)).copyTo(frame_mod(Rect(sx, 0, dx2, dy2)));
      addWeighted(frame_mod, 1.0, *fr, 1.0, 0.0, *fr);
   }
}
