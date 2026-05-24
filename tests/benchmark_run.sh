#!/bin/bash

echo "Starting NetMux Benchmarking Suite..."

# Cleanup
rm -f BENCHMARK_RESULTS.csv

# Start Server in background
./NetMux --server --port 9999 --bench > server_bench.log 2>&1 &
SERVER_PID=$!

sleep 2

# Start 5 Clients in background
for i in {1..5}
do
    ./NetMux --client 127.0.0.1 --port 9999 --bench > client_$i.log 2>&1 &
    CLIENT_PIDS[$i]=$!
done

echo "Benchmark running for 30 seconds..."
sleep 30

# Cleanup
kill $SERVER_PID
for pid in ${CLIENT_PIDS[*]}
do
    kill $pid
done

echo "Benchmark complete. Results saved in BENCHMARK_RESULTS.csv"
