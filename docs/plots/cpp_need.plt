set title "Performance of need()."

set xlabel "Sizeof need()"
#set xrange [0.1:100]
set logscale x
#set xrange [:1000]

# First line specification refers to major grid lines in both x and y, the second to minor grid lines in x and y.
set grid xtics mxtics ytics mytics linetype -1 linewidth 1, linetype 0 linewidth 1

set logscale y
set ylabel "Relative Performance (µs/sizeof need())"
# set yrange [1:]
# set ytics 8,35,3

#set logscale y2
#set y2label "Cache Misses"
# set y2range [1:1e9]
#set y2tics

set pointsize 1
set datafile separator whitespace
# Curve fit
#cost(x) = a + (b / (x/1024))
#fit cost(x) "data/perf_size_of.dat" using 1:2 via a,b

#set key right
#set key off

set terminal png size 800,500           # choose the file format

set output "images/cpp_need.png"   # choose the output device

plot "data/cpp_need.dat" using 1:(1e3 * $8 / ($7 / $1)) t "Time (µs)." with linespoints axes x1y1 pt 3 lw 2, \
    "data/cpp_need.dat" using 1:(0.2 + 1e-4 * $1) t "y = 0.2 + 1e-4 * x" with lines axes x1y1 lw 2

reset
