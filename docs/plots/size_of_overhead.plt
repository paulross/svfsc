set title "Memory Overhead."

set xlabel "Number of Blocks"
set xrange [1:1e4]
set logscale x
#set xrange [:1000]

# First line specification refers to major grid lines in both x and y, the second to minor grid lines in x and y.
set grid xtics mxtics ytics mytics linetype -1 linewidth 1, linetype 0 linewidth 1

set logscale y
set ylabel "Additional Memory Overhead (bytes)"
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
#fit cost(x) "data/size_of_overhead.dat" using 1:2 via a,b

set key left
#set key off

set terminal png size 800,500           # choose the file format

set output "images/size_of_overhead.png"   # choose the output device

#plot "data/size_of_overhead.dat" using 2:($2 * 64) t "64 bytes per block" with linespoints axes x1y1 pt 3 lw 4, \
    "data/size_of_overhead.dat" using 2:5 t "Actual overhead (bytes)" with linespoints axes x1y1 pt 3 lw 2

#plot "data/size_of_overhead.dat" using 2:($2 * 64) t "64 bytes per block" with lines axes x1y1 lw 4, \
    "data/size_of_overhead.dat" using 2:5 t "Actual overhead (bytes)" with lines axes x1y1 lw 2

plot "data/size_of_overhead.dat" using 2:5 t "Actual overhead (bytes)" with lines axes x1y1 lw 4, \
    "data/size_of_overhead.dat" using 2:($2 * 64) t "64 bytes per block" with lines axes x1y1 lw 2


reset
