#!/usr/bin/env python3
"""Plot latency report from nanomatch replay CSV output."""

import argparse
import csv
from pathlib import Path

import matplotlib.pyplot as plt


def percentile(sorted_values, p):
    if not sorted_values:
        return 0
    idx = int(p * (len(sorted_values) - 1))
    return sorted_values[idx]


def main():
    parser = argparse.ArgumentParser(description="Plot NANOMATCH latency CSV")
    parser.add_argument("--in", dest="input_csv", required=True, help="Input latency CSV path")
    parser.add_argument("--out", dest="output_png", default="latency_report.png", help="Output image path")
    parser.add_argument("--tail", type=int, default=5000, help="Number of latest samples to show in line chart")
    args = parser.parse_args()

    cycles = []
    with open(args.input_csv, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            cycles.append(int(row["cycles"]))

    if not cycles:
        raise SystemExit("No latency samples found in CSV.")

    sorted_cycles = sorted(cycles)
    p50 = percentile(sorted_cycles, 0.50)
    p90 = percentile(sorted_cycles, 0.90)
    p99 = percentile(sorted_cycles, 0.99)
    avg = sum(cycles) / len(cycles)

    show = cycles[-args.tail :] if len(cycles) > args.tail else cycles

    fig, axes = plt.subplots(2, 1, figsize=(12, 8))
    fig.suptitle("NANOMATCH Latency Report (cycles)")

    axes[0].plot(range(len(show)), show, linewidth=1.0, color="tab:blue")
    axes[0].set_title(f"Per-event latency (last {len(show)} samples)")
    axes[0].set_xlabel("Event index (window)")
    axes[0].set_ylabel("Cycles")
    axes[0].grid(alpha=0.3)

    axes[1].hist(cycles, bins=80, color="tab:green", alpha=0.85)
    axes[1].set_title("Latency distribution")
    axes[1].set_xlabel("Cycles")
    axes[1].set_ylabel("Frequency")
    axes[1].grid(alpha=0.3)

    text = f"count={len(cycles)}  avg={avg:.1f}  p50={p50}  p90={p90}  p99={p99}"
    fig.text(0.02, 0.02, text, fontsize=10)

    output = Path(args.output_png)
    output.parent.mkdir(parents=True, exist_ok=True)
    fig.tight_layout(rect=(0, 0.04, 1, 0.97))
    fig.savefig(output, dpi=140)
    print(f"Saved plot to {output}")
    print(text)


if __name__ == "__main__":
    main()
