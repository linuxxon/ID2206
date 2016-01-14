## Gnuplot scriptfile for plotting performance data of malloc versions
#
# Used for evaluation of course work for ID2206 at KTH
# Author: Rasmus Linusson
# Last modified: 8/1-2016
#####

set autoscale
unset log
unset label
set xtic auto
set ytic auto
set grid
set terminal png

## Plot memory
set xlabel  "Number of allocations"
set ylabel  "Bytes allocated from OS"

set output "memory_bestcase.png"
set title   "Memory usage vs. number of allocations - Best case"
plot    "best-case-0.dat" using 1:2 with linespoints title "System",\
        "best-case-1.dat" using 1:2 with linespoints title "First fit",\
        "best-case-2.dat" using 1:2 with linespoints title "Best fit",\
        "best-case-3.dat" using 1:2 with linespoints title "Worst fit",\
        "best-case-4.dat" using 1:2 with linespoints title "Quick fit"

set output "memory_worstcase.png"
set title   "Memory usage vs. number of allocations - Worst case"
plot    "worst-case-0.dat" using 1:2 with linespoints title "System",\
        "worst-case-1.dat" using 1:2 with linespoints title "First fit",\
        "worst-case-2.dat" using 1:2 with linespoints title "Best fit",\
        "worst-case-3.dat" using 1:2 with linespoints title "Worst fit",\
        "worst-case-4.dat" using 1:2 with linespoints title "Quick fit"

set output "memory_averagecase.png"
set title   "Memory usage vs. number of allocations - Average case"
plot    "average-case-0.dat" using 1:2 with linespoints title "System",\
        "average-case-1.dat" using 1:2 with linespoints title "First fit",\
        "average-case-2.dat" using 1:2 with linespoints title "Best fit",\
        "average-case-3.dat" using 1:2 with linespoints title "Worst fit",\
        "average-case-4.dat" using 1:2 with linespoints title "Quick fit"

## Plot time
set xlabel  "Number of allocations"
set ylabel  "CPU Time (user+kernel)"

set output "time_bestcase.png"
set title   "Execution time vs. number of allocations - Best case"
plot    "best-case-0.dat" using 1:3:4:5 with errorlines title "System",\
        "best-case-1.dat" using 1:3:4:5 with errorlines title "First fit",\
        "best-case-2.dat" using 1:3:4:5 with errorlines title "Best fit",\
        "best-case-3.dat" using 1:3:4:5 with errorlines title "Worst fit",\
        "best-case-4.dat" using 1:3:4:5 with errorlines title "Quick fit"

set output "time_worstcase.png"
set title   "Execution time vs. number of allocations - Worst case"
plot    "worst-case-0.dat" using 1:3:4:5 with errorlines title "System",\
        "worst-case-1.dat" using 1:3:4:5 with errorlines title "First fit",\
        "worst-case-2.dat" using 1:3:4:5 with errorlines title "Best fit",\
        "worst-case-3.dat" using 1:3:4:5 with errorlines title "Worst fit",\
        "worst-case-4.dat" using 1:3:4:5 with errorlines title "Quick fit"

set output "time_averagecase.png"
set title   "Execution time vs. number of allocations - Average case"
plot    "average-case-0.dat" using 1:3:4:5 with errorlines title "System",\
        "average-case-1.dat" using 1:3:4:5 with errorlines title "First fit",\
        "average-case-2.dat" using 1:3:4:5 with errorlines title "Best fit",\
        "average-case-3.dat" using 1:3:4:5 with errorlines title "Worst fit",\
        "average-case-4.dat" using 1:3:4:5 with errorlines title "Quick fit"
