set title "Writing 1MB With Different Write Sizes."

set xlabel "Write Size (bytes)"
#set xrange [0.1:100]
set logscale x
#set xrange [:1000]

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

set output "images/cpp_1mb_write.png"   # choose the output device

plot "data/cpp_1mb_write_coalesced.dat" using 1:($7 / 1000) t "Time to Write (Coalesced)." with linespoints axes x1y1 pt 3 lw 2, \
    "data/cpp_1mb_write_uncoalesced.dat" using 1:($7 / 1000) t "Time to Write (Not Coalesced)." with linespoints axes x1y1 pt 3 lw 2

# set output "images/cpp_1mb_write_block_size_time.png"   # choose the output device

# plot "data/cpp_1mb_write_coalesced.dat" using 1:($7 * 1024**2 / $1) t "Time/block (Coalesced)." with linespoints axes x1y1 pt 3 lw 2, \
    "data/cpp_1mb_write_uncoalesced.dat" using 1:($7 * 1024**2 / $1) t "Time/block (Not Coalesced)." with linespoints axes x1y1 pt 3 lw 2

reset
