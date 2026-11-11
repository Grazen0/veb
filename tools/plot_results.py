#!/usr/bin/env python3

import argparse
import re
import xml.etree.ElementTree as ET
from pathlib import Path

import matplotlib.pyplot as plt

BENCHMARK_RE = re.compile(r"^(.*?) \[W=(\d+)\] \[N=(\d+)\]$")


def read_results(filename):
    results = {}
    root = ET.parse(filename).getroot()
    for benchmark in root.findall(".//BenchmarkResults"):
        match = BENCHMARK_RE.match(benchmark.get("name", ""))
        mean = benchmark.find("mean")
        if match is None or mean is None:
            continue

        operation, width, size = match.groups()
        mean_ns = float(mean.get("value"))
        low_ns = float(mean.get("lowerBound", mean_ns))
        high_ns = float(mean.get("upperBound", mean_ns))
        results.setdefault(operation, {}).setdefault(int(width), []).append(
            (int(size), mean_ns, low_ns, high_ns)
        )

    if not results:
        raise ValueError(f"No Catch2 benchmark results found in {filename}")

    for by_width in results.values():
        for points in by_width.values():
            points.sort(key=lambda point: point[0])
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("xml", nargs="?", type=Path, default=Path("results.xml"))
    parser.add_argument("-o", "--output", type=Path, default=Path("results.png"))
    args = parser.parse_args()

    results = read_results(args.xml)
    operations = sorted(results)
    columns = 2
    rows = (len(operations) + columns - 1) // columns
    fig, axes = plt.subplots(rows, columns, figsize=(12, 4 * rows), squeeze=False)

    for axis, operation in zip(axes.flat, operations):
        for width, points in sorted(results[operation].items()):
            sizes = [point[0] for point in points]
            # Convert nanoseconds to milliseconds; show Catch2's 95% CI.
            means = [point[1] / 1e6 for point in points]
            lower = [(point[1] - point[2]) / 1e6 for point in points]
            upper = [(point[3] - point[1]) / 1e6 for point in points]
            axis.errorbar(
                sizes,
                means,
                yerr=[lower, upper],
                marker="o",
                capsize=2,
                linewidth=1.5,
                label=f"W={width} bits",
            )

        axis.set_title(operation)
        axis.set_xlabel("Number of elements (N)")
        axis.set_ylabel("Mean time (ms, 95% CI)")
        axis.set_xscale("log")
        axis.grid(True, which="both", linestyle=":", alpha=0.5)
        axis.legend(title="Key width")

    for axis in axes.flat[len(operations) :]:
        axis.set_visible(False)

    fig.suptitle("Van Emde Boas benchmark results", fontsize=16)
    fig.tight_layout()
    fig.savefig(args.output, dpi=180, bbox_inches="tight")
    print(f"Saved plot to {args.output}")


if __name__ == "__main__":
    main()
