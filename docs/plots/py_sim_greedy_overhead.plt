set title "Overhead of a Greedy Read."

set xlabel "Size of Greedy Read (bytes)"
#set xrange [0:1e6]
set logscale x
#set xrange [0.01:100]

# First line specification refers to major grid lines in both x and y, the second to minor grid lines in x and y.
set grid xtics mxtics ytics mytics linetype -1 linewidth 1, linetype 0 linewidth 1

set ylabel "Bytes Read Relative to Minimal Case (%)"
set logscale y
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

set title "Overhead of a Greedy Read in Percent of Data."

set terminal png size 800,500           # choose the file format

set output "images/py_sim_greedy_overhead_percent.png"   # choose the output device

plot "data/py_sim_greedy_CMU-1.dat" using 1:(100 * $7 / $6) t "CMU-1" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-001.dat" using 1:(100 * $7 / $6) t "TUPAC-TR-001" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-002.dat" using 1:(100 * $7 / $6) t "TUPAC-TR-002" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-003.dat" using 1:(100 * $7 / $6) t "TUPAC-TR-003" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-004.dat" using 1:(100 * $7 / $6) t "TUPAC-TR-004" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-005.dat" using 1:(100 * $7 / $6) t "TUPAC-TR-005" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-006.dat" using 1:(100 * $7 / $6) t "TUPAC-TR-006" with linespoints axes x1y1 pt 3 lw 2

set output "images/py_sim_greedy_total_bytes_read.png"   # choose the output device
set title "Total Memory used by the SVF Cache."
set ylabel "Total Bytes Read (Mb)"
#set logscale x

plot "data/py_sim_greedy_CMU-1.dat" using 1:($7 / 1024**2) t "CMU-1" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-001.dat" using 1:($7 / 1024**2) t "TUPAC-TR-001" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-002.dat" using 1:($7 / 1024**2) t "TUPAC-TR-002" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-003.dat" using 1:($7 / 1024**2) t "TUPAC-TR-003" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-004.dat" using 1:($7 / 1024**2) t "TUPAC-TR-004" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-005.dat" using 1:($7 / 1024**2) t "TUPAC-TR-005" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-006.dat" using 1:($7 / 1024**2) t "TUPAC-TR-006" with linespoints axes x1y1 pt 3 lw 2

set output "images/py_sim_greedy_overhead.png"   # choose the output device
set title "Extra Memory used by the SVF Cache Above the Data Size."
set ylabel "Overhead (Mb)"
#set logscale x

plot "data/py_sim_greedy_CMU-1.dat" using 1:(($9 - $6) / 1024**2) t "CMU-1" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-001.dat" using 1:(($9 - $6) / 1024**2) t "TUPAC-TR-001" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-002.dat" using 1:(($9 - $6) / 1024**2) t "TUPAC-TR-002" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-003.dat" using 1:(($9 - $6) / 1024**2) t "TUPAC-TR-003" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-004.dat" using 1:(($9 - $6) / 1024**2) t "TUPAC-TR-004" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-005.dat" using 1:(($9 - $6) / 1024**2) t "TUPAC-TR-005" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-006.dat" using 1:(($9 - $6) / 1024**2) t "TUPAC-TR-006" with linespoints axes x1y1 pt 3 lw 2

set output "images/py_sim_greedy_overhead_cf_time.png"   # choose the output device
set ylabel "Relative time to 64kB Greedy Length"
#set logscale x

set logscale y2
set y2label "Additional Memory Overhead (MB)"
#set y2range [1:1e9]
set y2tics

set key top

# Plot the relative time relative to 64kB greedy length then the memory overhead.
#plot \
    "data/py_sim_greedy_CMU-1.dat" using 1:($2 / 417.1) t "Time CMU-1" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-001.dat" using 1:($2 / 3283.6) t "Time TUPAC-TR-001" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-002.dat" using 1:($2 / 584.4) t "Time TUPAC-TR-002" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-003.dat" using 1:($2 / 516.6) t "Time TUPAC-TR-003" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-004.dat" using 1:($2 / 1153.6) t "Time TUPAC-TR-004" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-005.dat" using 1:($2 / 822.9) t "Time TUPAC-TR-005" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-006.dat" using 1:($2 / 1012.4) t "Time TUPAC-TR-006" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_CMU-1.dat" using 1:(($9 - $6) / 1024**2) t "Memory CMU-1" with linespoints axes x1y2 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-001.dat" using 1:(($9 - $6) / 1024**2) t "Memory TUPAC-TR-001" with linespoints axes x1y2 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-002.dat" using 1:(($9 - $6) / 1024**2) t "Memory TUPAC-TR-002" with linespoints axes x1y2 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-003.dat" using 1:(($9 - $6) / 1024**2) t "Memory TUPAC-TR-003" with linespoints axes x1y2 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-004.dat" using 1:(($9 - $6) / 1024**2) t "Memory TUPAC-TR-004" with linespoints axes x1y2 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-005.dat" using 1:(($9 - $6) / 1024**2) t "Memory TUPAC-TR-005" with linespoints axes x1y2 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-006.dat" using 1:(($9 - $6) / 1024**2) t "Memory TUPAC-TR-006" with linespoints axes x1y2 pt 3 lw 2

# Reduced set

set title "Read Time and Memory Overhead with Different SVF Cache Greedy Sizes."

set key top center
set yrange [0.1:1e5]
set y2range [1e-4:1e2]

plot \
    "data/py_sim_greedy_CMU-1.dat" using 1:($2 / 417.1) t "Time CMU-1 [Left]" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-001.dat" using 1:($2 / 3283.6) t "Time TUPAC-TR-001 [Left]" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-002.dat" using 1:($2 / 584.4) t "Time TUPAC-TR-002 [Left]" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_CMU-1.dat" using 1:(($9 - $6) / 1024**2) t "Memory CMU-1 [Right]" with linespoints axes x1y2 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-001.dat" using 1:(($9 - $6) / 1024**2) t "Memory TUPAC-TR-001 [Right]" with linespoints axes x1y2 pt 3 lw 2, \
    "data/py_sim_greedy_TUPAC-TR-002.dat" using 1:(($9 - $6) / 1024**2) t "Memory TUPAC-TR-002 [Right]" with linespoints axes x1y2 pt 3 lw 2

reset
