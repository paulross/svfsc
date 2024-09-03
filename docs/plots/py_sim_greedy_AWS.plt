set title "Simulated Time to Read TIFF Metadata from an AWS Server. Selected Files.\nNetwork Latency 100ms Bandwidth 8 Mbps. Server Seek 10 GB/s Read 50 MB/s."

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

set pointsize 1
set datafile separator whitespace
# Curve fit
#cost(x) = a + (b / (x/1024))
#fit cost(x) "data/perf_size_of.dat" using 1:2 via a,b

set key right
#set key off

# Plot of time

set terminal png size 800,500           # choose the file format
set output "images/py_sim_greedy_AWS.png"   # choose the output device

plot "data/py_sim_greedy_AWS_CMU-1.dat" using 1:($2 / 1000) t "CMU-1" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_TUPAC-TR-001.dat" using 1:($2 / 1000) t "TUPAC-TR-001" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_TUPAC-TR-002.dat" using 1:($2 / 1000) t "TUPAC-TR-002" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_TUPAC-TR-003.dat" using 1:($2 / 1000) t "TUPAC-TR-003" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_TUPAC-TR-004.dat" using 1:($2 / 1000) t "TUPAC-TR-004" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_TUPAC-TR-005.dat" using 1:($2 / 1000) t "TUPAC-TR-005" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_TUPAC-TR-006.dat" using 1:($2 / 1000) t "TUPAC-TR-006" with linespoints axes x1y1 pt 3 lw 2

reset

# Plot of cost
set title "Simulated Cost to Read TIFF Metadata from an AWS Server. Selected Files.\nCost GET 0.000500 (per 1000 GET requests) egress 0.100 (per GB)."

set xlabel "Size of Greedy Read (bytes)"
#set xrange [1:1e6]
set logscale x

set logscale y
set ylabel "Cost"
set yrange [:10]

# First line specification refers to major grid lines in both x and y, the second to minor grid lines in x and y.
set grid xtics mxtics ytics mytics linetype -1 linewidth 1, linetype 0 linewidth 1

set terminal png size 800,500           # choose the file format
set output "images/py_sim_greedy_AWS_cost.png"   # choose the output device

set logscale y2
set y2label "Cost Ratio (Download / Read Metadata)"
set y2range [0.1:1e5]
set y2tics

set key top right

#plot "data/py_sim_greedy_AWS_CMU-1.dat" using 1:12 t "CMU-1 GET Cost" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_CMU-1.dat" using 1:13 t "CMU-1 Egress Cost" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_CMU-1.dat" using 1:14 t "CMU-1 Total Cost" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_CMU-1.dat" using 1:15 t "CMU-1 Download Cost" with linespoints axes x1y1 pt 3 lw 2#, \
    "data/py_sim_greedy_AWS_TUPAC-TR-001.dat" using 1:($2 / 1000) t "TUPAC-TR-001" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_TUPAC-TR-002.dat" using 1:($2 / 1000) t "TUPAC-TR-002" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_TUPAC-TR-003.dat" using 1:($2 / 1000) t "TUPAC-TR-003" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_TUPAC-TR-004.dat" using 1:($2 / 1000) t "TUPAC-TR-004" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_TUPAC-TR-005.dat" using 1:($2 / 1000) t "TUPAC-TR-005" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_TUPAC-TR-006.dat" using 1:($2 / 1000) t "TUPAC-TR-006" with lines axes x1y1 pt 3 lw 2

#plot "data/py_sim_greedy_AWS_TUPAC-TR-001.dat" using 1:12 t "TUPAC-TR-001 GET Cost" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_TUPAC-TR-001.dat" using 1:13 t "TUPAC-TR-001 Egress Cost" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_TUPAC-TR-001.dat" using 1:14 t "TUPAC-TR-001 Total Cost" with linespoints axes x1y1 pt 3 lw 2, \
    "data/py_sim_greedy_AWS_TUPAC-TR-001.dat" using 1:15 t "TUPAC-TR-001 Download Cost" with lines axes x1y1 lw 2

#plot "data/py_sim_greedy_AWS_CMU-1.dat" using 1:12 t "CMU-1 GET Cost" with linespoints axes x1y1 pt 3 lw 2, \
     "data/py_sim_greedy_AWS_CMU-1.dat" using 1:13 t "CMU-1 Egress Cost" with linespoints axes x1y1 pt 3 lw 2, \
     "data/py_sim_greedy_AWS_CMU-1.dat" using 1:14 t "CMU-1 Total Cost" with linespoints axes x1y1 pt 3 lw 2, \
     "data/py_sim_greedy_AWS_CMU-1.dat" using 1:15 t "CMU-1 Download Cost" with lines axes x1y1 lw 2, \
     "data/py_sim_greedy_AWS_TUPAC-TR-001.dat" using 1:12 t "TUPAC-TR-001 GET Cost" with linespoints axes x1y1 pt 3 lw 2, \
     "data/py_sim_greedy_AWS_TUPAC-TR-001.dat" using 1:13 t "TUPAC-TR-001 Egress Cost" with linespoints axes x1y1 pt 3 lw 2, \
     "data/py_sim_greedy_AWS_TUPAC-TR-001.dat" using 1:14 t "TUPAC-TR-001 Total Cost" with linespoints axes x1y1 pt 3 lw 2, \
     "data/py_sim_greedy_AWS_TUPAC-TR-001.dat" using 1:15 t "TUPAC-TR-001 Download Cost" with lines axes x1y1 lw 2

plot "data/py_sim_greedy_AWS_CMU-1.dat" using 1:14 t "CMU-1 Metadata Cost" with lines axes x1y1 lw 3, \
     "data/py_sim_greedy_AWS_CMU-1.dat" using 1:15 t "CMU-1 Download Cost" with lines axes x1y1 lw 3, \
     "data/py_sim_greedy_AWS_CMU-1.dat" using 1:($15 / $14) t "CMU-1 Ratio [Right]" with lines axes x1y2 lw 3, \
     "data/py_sim_greedy_AWS_TUPAC-TR-001.dat" using 1:14 t "TUPAC-TR-001 Metadata Cost" with lines axes x1y1 lw 3, \
     "data/py_sim_greedy_AWS_TUPAC-TR-001.dat" using 1:15 t "TUPAC-TR-001 Download Cost" with lines axes x1y1 lw 3, \
     "data/py_sim_greedy_AWS_TUPAC-TR-001.dat" using 1:($15 / $14) t "TUPAC-TR-001 Ratio [Right]" with lines axes x1y2 lw 3

reset
