#!/bin/bash

output_file="performance_results.csv"
echo "version,threads,time" > $output_file

# Versions array
declare -a versions=("static" "dynamic")

# Thread counts
declare -a thread_counts=(1 2 4 8 16 32)

# Loop over each version
for version in static dynamic; do
  for threads in 1 2 4 8 16 32; do
    echo "Running $version allocation with $threads threads"
    output=$(srun --nodes=1 ./s $threads $version)
    echo "$output"
    time=$(echo "$output" | grep 'Time' | awk '{print $NF}')
    echo "Captured Time: $time"
    echo "$version,$threads,$time" >> $output_file
  done
done

