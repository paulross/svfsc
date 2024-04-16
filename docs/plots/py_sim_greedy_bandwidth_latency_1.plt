set title "Effect of Network Bandwidth, Latency 1 ms. Server Seek 10 GB/s Read 50 MB/s."

set xlabel "Size of Greedy Read (bytes)"
#set xrange [1:1e6]
set logscale x
#set xrange [0.01:100]

# First line specification refers to major grid lines in both x and y, the second to minor grid lines in x and y.
set grid xtics mxtics ytics mytics linetype -1 linewidth 1, linetype 0 linewidth 1

set logscale y
set ylabel "Time (s)"
# set yrange [1:]
# set ytics 8,35,3

#set logscale y2
#set y2label "Cache Nisses"
# set y2range [1:1e9]
#set y2tics

set pointsize 1
set datafile separator whitespace
# Curve fit
#cost(x) = a + (b / (x/1024))
#fit cost(x) "data/perf_size_of.dat" using 1:2 via a,b

set key right
#set key off

set terminal png size 800,500           # choose the file format

set output "images/py_sim_greedy_bandwidth_latency_1.png"   # choose the output device

plot \
    "data/py_sim_greedy_CMU-1_bandwidth_10_latency_1.dat" using 1:($2 / 1000) t "Bandwidth 10 (Mbps)" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_CMU-1_bandwidth_50_latency_1.dat" using 1:($2 / 1000) t "Bandwidth 50 (Mbps)" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_CMU-1_bandwidth_500_latency_1.dat" using 1:($2 / 1000) t "Bandwidth 500 (Mbps)" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_CMU-1_bandwidth_0_latency_1.dat" using 1:($2 / 1000) t "Bandwidth Infinite (Mbps)" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_CMU-1_ZLIB.dat" using 1:($2 / 1000) t "ZLIB" with linespoints axes x1y1 pt 3 lw 2, \

reset
