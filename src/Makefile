BIN = kireco kirana ionreco ionana

.PHONY: all
all: $(BIN)

kireco: kireco.o
	g++ -O3 -Wall -o $@ $< `pkg-config --libs opencv` -lwebcam

kirana: kirana.o
	g++ -O3 -Wall -o $@ $< `pkg-config --libs opencv`

ionreco: ionreco.o
	g++ -O3 -Wall -o $@ $< `pkg-config --libs opencv` -lwebcam

ionana: ionana.o
	g++ -O3 -Wall -o $@ $< `pkg-config --libs opencv`

.cc.o:
	g++ -O3 -Wall -o $@ -c $< `pkg-config --cflags opencv`

.PHONY: install clean
install: $(BIN)
	cp -a $^ ../

clean:
	rm -f $(BIN) *.o

##
## Memo
##
# aptitude install libopencv-dev libwebcam0-dev
#
# Reference:
#   Reverse playback:
#     https://www.opencv-srf.com/2017/12/play-video-file-backwards.html
#   User Interface: 
#     http://opencv.jp/opencv-2.1/cpp/user_interface.html 


