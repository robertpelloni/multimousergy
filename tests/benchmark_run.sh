#!/bin/bash
echo "Starting NetMux Benchmarking Suite..."
rm -f BENCHMARK_RESULTS.csv
./NetMux --server --port 9999 --bench > server_bench.log 2>&1 &
SERVER_PID=$!
sleep 2
CLIENT_PIDS=()
for i in {1..$1}
do
    ./NetMux --client 127.0.0.1 --port 9999 --bench > client_$i.log 2>&1 &
    CLIENT_PIDS+=($!)
done
echo "Benchmark running for 10 seconds..."
sleep 10
kill $SERVER_PID
for pid in ${CLIENT_PIDS[@]}
do
    kill $pid
done
wait
echo "Benchmark complete."
