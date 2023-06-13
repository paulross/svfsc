set title "Simulated Time to Read TIFF Metadata from a Remote Server. Selected Files.\nNetwork Latency 10ms Bandwidth 50 Mbps. Server Seek 10 GB/s Read 50 MB/s."

set xlabel "Size of Greedy Read (bytes)"
set xrange [1:1e6]
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

set output "images/py_sim_greedy.png"   # choose the output device

plot "data/py_sim_greedy_CMU-1.dat" using 1:($2 / 1000) t "CMU-1" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-001.dat" using 1:($2 / 1000) t "TUPAC-TR-001" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-002.dat" using 1:($2 / 1000) t "TUPAC-TR-002" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-003.dat" using 1:($2 / 1000) t "TUPAC-TR-003" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-004.dat" using 1:($2 / 1000) t "TUPAC-TR-004" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-005.dat" using 1:($2 / 1000) t "TUPAC-TR-005" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-006.dat" using 1:($2 / 1000) t "TUPAC-TR-006" with linespoints axes x1y1 pt 3 lw 2

reset
