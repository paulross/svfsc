set title "Over Head of a Greedy Read."

set xlabel "Size of Greedy Read (bytes)"
#set xrange [0.1:100]
#set logscale x
#set xrange [0.01:100]

# First line specification refers to major grid lines in both x and y, the second to minor grid lines in x and y.
set grid xtics mxtics ytics mytics linetype -1 linewidth 1, linetype 0 linewidth 1

#set logscale y
set ylabel "Bytes Read Relative to Minimal Case (%)"
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
#fit cost(x) "perf_size_of.dat" using 1:2 via a,b

set key right
#set key off

set terminal png size 800,500           # choose the file format

set output "py_sim_greedy_overhead_percent.png"   # choose the output device

plot "py_sim_greedy_CMU-1.dat" using 1:(100 * $7 / $6) t "CMU-1" with linespoints axes x1y1 pt 3 lw 2, \
    "py_sim_greedy_TUPAC-TR-001.dat" using 1:(100 * $7 / $6) t "TUPAC-TR-001" with linespoints axes x1y1 pt 3 lw 2, \
    "py_sim_greedy_TUPAC-TR-002.dat" using 1:(100 * $7 / $6) t "TUPAC-TR-002" with linespoints axes x1y1 pt 3 lw 2, \
    "py_sim_greedy_TUPAC-TR-003.dat" using 1:(100 * $7 / $6) t "TUPAC-TR-003" with linespoints axes x1y1 pt 3 lw 2, \
    "py_sim_greedy_TUPAC-TR-004.dat" using 1:(100 * $7 / $6) t "TUPAC-TR-004" with linespoints axes x1y1 pt 3 lw 2, \
    "py_sim_greedy_TUPAC-TR-005.dat" using 1:(100 * $7 / $6) t "TUPAC-TR-005" with linespoints axes x1y1 pt 3 lw 2, \
    "py_sim_greedy_TUPAC-TR-006.dat" using 1:(100 * $7 / $6) t "TUPAC-TR-006" with linespoints axes x1y1 pt 3 lw 2

set output "py_sim_greedy_overhead.png"   # choose the output device
set ylabel "Total Bytes Read (Mb)"
#set logscale x

plot "py_sim_greedy_CMU-1.dat" using 1:($7 / 1024**2) t "CMU-1" with linespoints axes x1y1 pt 3 lw 2, \
    "py_sim_greedy_TUPAC-TR-001.dat" using 1:($7 / 1024**2) t "TUPAC-TR-001" with linespoints axes x1y1 pt 3 lw 2, \
    "py_sim_greedy_TUPAC-TR-002.dat" using 1:($7 / 1024**2) t "TUPAC-TR-002" with linespoints axes x1y1 pt 3 lw 2, \
    "py_sim_greedy_TUPAC-TR-003.dat" using 1:($7 / 1024**2) t "TUPAC-TR-003" with linespoints axes x1y1 pt 3 lw 2, \
    "py_sim_greedy_TUPAC-TR-004.dat" using 1:($7 / 1024**2) t "TUPAC-TR-004" with linespoints axes x1y1 pt 3 lw 2, \
    "py_sim_greedy_TUPAC-TR-005.dat" using 1:($7 / 1024**2) t "TUPAC-TR-005" with linespoints axes x1y1 pt 3 lw 2, \
    "py_sim_greedy_TUPAC-TR-006.dat" using 1:($7 / 1024**2) t "TUPAC-TR-006" with linespoints axes x1y1 pt 3 lw 2

reset
