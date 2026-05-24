import csv
import sys

def percentile(data, percentile):
    size = len(data)
    if size == 0: return 0
    sorted_data = sorted(data)
    index = (size - 1) * percentile / 100
    lower = int(index)
    upper = lower + 1
    weight = index - lower
    if upper >= size:
        return sorted_data[lower]
    return sorted_data[lower] * (1 - weight) + sorted_data[upper] * weight

def analyze(filename):
    rtts = []
    e2e = []
    frames = []

    try:
        with open(filename, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                rtt = float(row['AvgRTT(ms)'])
                latency = float(row['AvgE2ELatency(ms)'])
                frame = float(row['FrameDelta(ms)'])

                if rtt > 0: rtts.append(rtt)
                if latency != 0: e2e.append(abs(latency))
                if frame > 0: frames.append(frame)

        print(f"--- NetMux Performance Analysis ---")
        for name, data in [("RTT", rtts), ("E2E Latency", e2e), ("Frame Delta", frames)]:
            if not data:
                print(f"{name}: No valid data.")
                continue
            mean = sum(data) / len(data)
            print(f"{name} ({len(data)} samples):")
            print(f"  Mean: {mean:.3f} ms")
            print(f"  p50:  {percentile(data, 50):.3f} ms")
            print(f"  p95:  {percentile(data, 95):.3f} ms")
            print(f"  p99:  {percentile(data, 99):.3f} ms")
            print(f"  Max:  {max(data):.3f} ms")

    except Exception as e:
        print(f"Error analyzing data: {e}")

if __name__ == "__main__":
    analyze('BENCHMARK_RESULTS.csv')
