set title "Simulated Time to Read Synthetic File. Latency 10ms Bandwidth 50 Mbps."

set xlabel "Size of Greedy Read (bytes)"
#set xrange [0.1:100]
set logscale x
set xrange [10:1e5]

# First line specification refers to major grid lines in both x and y, the second to minor grid lines in x and y.
set grid xtics mxtics ytics mytics linetype -1 linewidth 1, linetype 0 linewidth 1

set logscale y
set ylabel "Time (s)"
# set yrange [1:]
# set ytics 8,35,3

set logscale y2
set y2label "Cache Nisses"
# set y2range [1:1e9]
set y2tics

set pointsize 1
set datafile separator whitespace
# Curve fit
#cost(x) = a + (b / (x/1024))
#fit cost(x) "data/perf_size_of.dat" using 1:2 via a,b

set key right
#set key off

set terminal png size 800,500           # choose the file format

set output "images/greedy_length_synthetic.png"   # choose the output device

plot "data/greedy_length_synthetic.dat" using 1:($2 / 1000) t "Time to Read (left axis)." with linespoints axes x1y1 pt 3 lw 2, \
    "data/greedy_length_synthetic.dat" using 1:4 t "Cache Misses (right axis)." with linespoints axes x1y2 pt 3 lw 2

reset
