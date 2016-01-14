#!/bin/bash
###############################
## Test executer for performance evaluation of custom malloc strategies
##
## Runs $1 amount of iterations
##
## Author: Rasmus Linusson
## Last modified: 07/01-2016
## For course ID2206 at KTH
###############################

## Test parameters
# Start with 1000 allocations
allocations=10000

# Each consecutive test adds 1000 allocations
allocation_increment=10000

# Test for 50 points
allocation_nr_increments=25

# Repeat test 'iterations' times
iterations=`seq 11`

# Test executable to run
#test_exec="./memory-test"
testcases="./best-case ./worst-case ./average-case"

## Fetch parameters from script arguments
## NOT IMPLEMENTED YET

# Ready a list of all test parameters
num_allocations=`awk "BEGIN{for(i=0; i < $allocation_nr_increments; i+=1) print $allocations+$allocation_increment*i}"`

# Clean make.log
echo "" > make.log

# Temp files
temp_iter="iter.tmp"
temp_res="res.tmp"

## Run tests for all five strategies and for all three cases
## best-case
## worst-case
## average-case
##
## 0 = Standard system malloc
## 1 = First fit
## 2 = Best  fit
## 3 = Worst fit
## 4 = Quick fit
for test_exec in $testcases; do
    for strategy in 0; do #`seq 1 4`; do
        echo "Strategy $strategy"

        # Make proper executable
        if [ $strategy -eq 0 ]; then
            make clean ${test_exec:2} STDMALLOC=true 2>&1 >> make.log
        else
            make clean ${test_exec:2} STDMALLOC=false STRATEGY=$strategy 2>&1 >> make.log
        fi

        # Check if make failed
        if [ $? -ne 0 ]; then
            echo "Make failed... exiting"
            exit $?
        fi

        file="${test_exec:2}-$strategy.dat"

        # Print header of data file
        printf "# Allocations\tMemory\tCPU Time\tMin\tMax\n" > $file

        ## Run test for each amount of allocations
        for allocation in $num_allocations; do

            if [ -e $temp_iter ]; then
                rm $temp_iter
            fi
            if [ -e $temp_res ]; then
                rm $temp_res
            fi

            # Repeat the test $iterations times as to get median and deviation
            for i in $iterations; do

                ## Run test with /usr/bin/time to get CPU time
                # Separate with newline and indicate with 'Time: ' as to be able
                # to differentiate time from output of the test with grep
                output=$(/usr/bin/time --format "Time: %U %S\n" $test_exec $allocation 2>&1)
                _memory=$(echo "$output" | grep -o "Memory usage: [0-9]*" | awk '{print $3}')
                _time=$(echo "$output" | grep -o "Time: [0-9]*.[0-9]* [0-9]*.[0-9]*" | awk '{print $2+$3}')

                printf "%d\t%11d\t%f\n" $allocation $_memory $_time >> $temp_iter
            done

            # Calculate median this is based on $iterations = seq 11
            median=$(sort -g -k1,3 $temp_iter | tee $temp_res | awk 'NR==6 {OFS=FS="\t"; print $1, $2, $3}')
            min=$(awk 'NR==1 {OFS=FS="\t"; print $3}' $temp_res)
            max=$(awk 'END {OFS=FS="\t"; print $3}' $temp_res)
            printf "%s\t%s\t%s\n" "$median" "$min" "$max" >> $file
        done
        echo "Strategy done"
    done
    # Clean tempfile
    if [ -e $temp_iter ]; then
        rm $temp_iter
    fi
done
