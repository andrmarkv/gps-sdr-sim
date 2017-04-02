ftp://cddis.gsfc.nasa.gov/gnss/data/daily/2017/016/17n/brdc0160.17n.Z
ftp://cddis.gsfc.nasa.gov/gnss/data/daily/2017/brdc/  

mkdir /tmp/ramdisk; chmod 777 /tmp/ramdisk; mount -t tmpfs -o size=526M tmpfs /tmp/ramdisk/; mkfifo /tmp/ramdisk/gpssim.bin

./gps-sdr-sim -s 2500000 -e brdc3500.16n -l 24.506449,54.372192,111

./gps-sdr-sim -s 2500000 -e brdc3500.16n -T

./gps-sdr-sim -s 2500000 -e brdc3500.16n -t 2016/12/15,12:00:00

./gps-sdr-sim -s 2500000 -e brdc0400.17n -t 2017/02/09,12:00:00
/usr/lib/uhd/examples/tx_samples_from_file --args="master_clock_rate=50e6" --file gpssim.bin --type short --rate 2500000 --freq 1575420000 --gain 45

#For HackRF
#!!! Make sure that -b 8 and -s 2600000 !!!
./gps-sdr-sim -b 8 -s 2600000 -e brdc3500.16n -t 2016/12/15,12:00:00 -l 24.495774,54.358074,1

hackrf_transfer -t gpssim.bin -f 1575420000 -s 2600000 -a 0 -x 30
hackrf_transfer -t gpssim.bin -f 1575420000 -s 2600000 -a 1 -x 30

pip install -U scikit-image

adb shell getevent | grep --line-buffered ^/ | tee /tmp/android-touch-events.log
awk '{printf "%s %d %d %d\n", substr($1, 1, length($1) -1), strtonum("0x"$2), strtonum("0x"$3), strtonum("0x"$4)}' /tmp/android-touch-events.log | xargs -l adb shell sendevent

adb shell screencap -p | sed 's/\r$//' > screen.png

python -m pip install --upgrade pip
pip install --user numpy scipy matplotlib ipython jupyter pandas sympy nose
adb shell screencap -p | sed 's/\r$//' > screen_$(date +'%s').png


sudo apt-get install --assume-yes build-essential cmake git
sudo apt-get install --assume-yes build-essential pkg-config unzip ffmpeg qtbase5-dev python-dev python3-dev python-numpy python3-numpy
sudo apt-get install --assume-yes libopencv-dev libgtk-3-dev libdc1394-22 libdc1394-22-dev libjpeg-dev libpng12-dev libtiff5-dev libjasper-dev
sudo apt-get install --assume-yes libavcodec-dev libavformat-dev libswscale-dev libxine2-dev libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev
sudo apt-get install --assume-yes libv4l-dev libtbb-dev libfaac-dev libmp3lame-dev libopencore-amrnb-dev libopencore-amrwb-dev libtheora-dev
sudo apt-get install --assume-yes libvorbis-dev libxvidcore-dev v4l-utils

cd /usr/include/linux
ln -s ../libv4l1-videodev.h videodev.h
mkdir /usr/include/ffmpeg
cd /usr/include/ffmpeg
ln -sf /usr/include/x86_64-linux-gnu/libavcodec/*.h ./
ln -sf /usr/include/x86_64-linux-gnu/libavformat/*.h ./
ln -sf /usr/include/x86_64-linux-gnu/libswscale/*.h ./

cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local -D WITH_TBB=ON -D WITH_V4L=ON -D WITH_QT=ON -D WITH_OPENGL=ON -D WITH_CUBLAS=ON -DCUDA_NVCC_FLAGS="-D_FORCE_INLINES" ..
cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local -D WITH_TBB=ON -D WITH_V4L=OFF -D WITH_LIBV4L=ON -D WITH_QT=ON -D WITH_OPENGL=ON -D WITH_CUBLAS=ON -DCUDA_NVCC_FLAGS="-D_FORCE_INLINES" ..    