./gps-sdr-sim -s 2500000 -e brdc3500.16n -l 24.506449,54.372192,111

./gps-sdr-sim -s 2500000 -e brdc3500.16n -T

./gps-sdr-sim -s 2500000 -e brdc3500.16n -t 2016/12/15,12:00:00

#For HackRF
#!!! Make sure that -b 8 and -s 2600000 !!!
./gps-sdr-sim -b 8 -s 2600000 -e brdc3500.16n -t 2016/12/15,12:00:00 -l 24.495774,54.358074,1
hackrf_transfer -t gpssim.bin -f 1575420000 -s 2600000 -a 1 -x 30