ftp://cddis.gsfc.nasa.gov/gnss/data/daily/2017/016/17n/brdc0160.17n.Z
ftp://cddis.gsfc.nasa.gov/gnss/data/daily/2017/brdc/  

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
