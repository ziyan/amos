#!/bin/bash
amosmotortune /dev/motor 115200 0.091 0.0025 0.018 100 1 > motortune.dat
echo -e "plot \"motortune.dat\" using 1:2 with lines, \"motortune.dat\" using 1:3 with lines\npause -1"> motortune.plot
gnuplot "motortune.plot"
rm -f motortune.dat motortune.plot
