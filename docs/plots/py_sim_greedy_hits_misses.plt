set title "Cache Hits and Misses."

set xlabel "Size of Greedy Read (bytes)"
#set xrange [0.1:100]
set logscale x
#set xrange [0.01:100]

# First line specification refers to major grid lines in both x and y, the second to minor grid lines in x and y.
set grid xtics mxtics ytics mytics linetype -1 linewidth 1, linetype 0 linewidth 1

set logscale y
set ylabel "Cache Hits and Misses"
#set yrange [0:1.1e5]
# set ytics 8,35,3

#set logscale y2
#set y2label "Cache Nisses"
#set y2range [0:1e5]
#set y2tics

set pointsize 1
set datafile separator whitespace
# Curve fit
#cost(x) = a + (b / (x/1024))
#fit cost(x) "data/perf_size_of.dat" using 1:2 via a,b

# set key bottom left
set key top right
#set key off

set terminal png size 800,500           # choose the file format

set output "images/py_sim_greedy_hits_misses.png"   # choose the output device

# Unused
#plot "data/py_sim_greedy_CMU-1.dat" using 1:3 t "CMU-1 Hits" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_CMU-1.dat" using 1:4 t "CMU-1 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-001.dat" using 1:3 t "TUPAC-TR-001 Hits" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-001.dat" using 1:4 t "TUPAC-TR-001 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-002.dat" using 1:3 t "TUPAC-TR-002 Hits" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-002.dat" using 1:4 t "TUPAC-TR-002 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-003.dat" using 1:3 t "TUPAC-TR-003 Hits" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-003.dat" using 1:4 t "TUPAC-TR-003 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-004.dat" using 1:3 t "TUPAC-TR-004 Hits" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-004.dat" using 1:4 t "TUPAC-TR-004 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-005.dat" using 1:3 t "TUPAC-TR-005 Hits" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-005.dat" using 1:4 t "TUPAC-TR-005 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-006.dat" using 1:3 t "TUPAC-TR-006 Hits" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-006.dat" using 1:4 t "TUPAC-TR-006 Miss" with linespoints axes x1y1 pt 3 lw 2

# All - unused
#plot "data/py_sim_greedy_TUPAC-TR-002.dat" using 1:3 t "TUPAC-TR-002 Hits" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-002.dat" using 1:4 t "TUPAC-TR-002 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-003.dat" using 1:3 t "TUPAC-TR-003 Hits" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-003.dat" using 1:4 t "TUPAC-TR-003 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-005.dat" using 1:3 t "TUPAC-TR-005 Hits" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-005.dat" using 1:4 t "TUPAC-TR-005 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-006.dat" using 1:3 t "TUPAC-TR-006 Hits" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-006.dat" using 1:4 t "TUPAC-TR-006 Miss" with linespoints axes x1y1 pt 3 lw 2

# Just 003 and 006
#plot "data/py_sim_greedy_TUPAC-TR-003.dat" using 1:3 t "TUPAC-TR-003 Hits" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-006.dat" using 1:3 t "TUPAC-TR-006 Hits" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-003.dat" using 1:4 t "TUPAC-TR-003 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-006.dat" using 1:4 t "TUPAC-TR-006 Miss" with linespoints axes x1y1 pt 3 lw 2

# Just 003 and 006, misses only
set title "Cache Misses. Selected Files."
set ylabel "Cache Misses"
set xrange [1:1e6]
plot "data/py_sim_greedy_CMU-1.dat" using 1:4 t "CMU-1 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-001.dat" using 1:4 t "TUPAC-TR-001 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-002.dat" using 1:4 t "TUPAC-TR-002 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-003.dat" using 1:4 t "TUPAC-TR-003 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-004.dat" using 1:4 t "TUPAC-TR-004 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-005.dat" using 1:4 t "TUPAC-TR-005 Miss" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-006.dat" using 1:4 t "TUPAC-TR-006 Miss" with linespoints axes x1y1 pt 3 lw 2

reset
