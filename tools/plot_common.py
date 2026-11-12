import re
import xml.etree.ElementTree as ET
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
OUTPUTS = ROOT / "outputs"
STRUCTURES = [
    "Veb",
    "std::priority_queue",
    "std::multiset",
    "boost::heap::binomial_heap",
    "boost::heap::fibonacci_heap",
]
COLORS = ["#4c78a8", "#f58518", "#54a24b", "#e45756", "#72b7b2"]


def benchmark_sections(xml_path):
    root = ET.parse(xml_path).getroot()
    found = False
    for testcase in root.findall("TestCase"):
        sections = testcase.findall("Section")
        if sections:
            for section in sections:
                points = _benchmark_points(section)
                if points:
                    found = True
                    yield section.get("name", "input"), points
        else:
            points = _benchmark_points(testcase)
            if points:
                found = True
                yield testcase.get("name", "benchmark"), points
    if not found:
        raise ValueError(f"No benchmark results found in {xml_path}")


def _benchmark_points(parent):
    points = []
    for result in parent.findall("BenchmarkResults"):
        mean = result.find("mean")
        if mean is None:
            continue
        value = float(mean.get("value")) / 1e6  # Catch2 XML stores nanoseconds.
        low = float(mean.get("lowerBound", mean.get("value"))) / 1e6
        high = float(mean.get("upperBound", mean.get("value"))) / 1e6
        points.append((result.get("name", "benchmark"), value, low, high))
    return points


def natural_key(value):
    return [
        int(part) if part.isdigit() else part.casefold()
        for part in re.split(r"(\d+)", value)
    ]


def style_axis(axis):
    axis.set_ylabel("Mean elapsed time (ms)")
    axis.grid(axis="y", linestyle=":", alpha=0.45)
    axis.set_axisbelow(True)
    axis.spines[["top", "right"]].set_visible(False)


def plot_structure_bars(xml_path, title):
    """Create one grouped-bar chart comparing queue structures per input."""
    data = list(benchmark_sections(xml_path))
    data.sort(key=lambda item: natural_key(item[0]))
    inputs = [Path(label).stem for label, _ in data]
    present_names = {name for _, points in data for name, *_ in points}
    names = [name for name in STRUCTURES if name in present_names]
    names.extend(sorted(present_names.difference(names)))

    fig_width = max(9, 1.45 * len(inputs) + 4)
    fig, axis = plt.subplots(figsize=(fig_width, 6))
    centers = np.arange(len(inputs), dtype=float)
    group_width = 0.82
    bar_width = group_width / max(1, len(names))

    for index, name in enumerate(names):
        means, lows, highs = [], [], []
        for _, points in data:
            match = next((point for point in points if point[0] == name), None)
            if match is None:
                means.append(np.nan)
                lows.append(0.0)
                highs.append(0.0)
            else:
                _, mean, low, high = match
                means.append(mean)
                lows.append(max(0.0, mean - low))
                highs.append(max(0.0, high - mean))

        offset = (index - (len(names) - 1) / 2) * bar_width
        axis.bar(
            centers + offset,
            means,
            width=bar_width * 0.92,
            yerr=np.array([lows, highs]),
            capsize=2,
            color=COLORS[index % len(COLORS)],
            label=name,
            linewidth=0,
        )

    axis.set_xticks(centers, inputs, rotation=25, ha="right")
    axis.set_xlabel("Test input")
    style_axis(axis)
    axis.set_title(title)
    handles, labels = axis.get_legend_handles_labels()
    fig.legend(
        handles,
        labels,
        title="Data structure",
        frameon=False,
        ncols=len(labels),
        loc="lower center",
        bbox_to_anchor=(0.5, 0.01),
    )
    fig.tight_layout(rect=(0, 0.12, 1, 1))
    output = OUTPUTS / f"{Path(xml_path).stem}.png"
    fig.savefig(output, dpi=180, bbox_inches="tight")
    plt.close(fig)
    print(f"Saved {output}")


def plot_veb(xml_path):
    """Make a separate N-scaling chart for each standalone Veb operation."""
    pattern = re.compile(r"^(.*?) \[W=(\d+)\] \[N=(\d+)\]$")
    results = {}
    root = ET.parse(xml_path).getroot()
    for testcase in root.findall("TestCase"):
        for benchmark in testcase.findall("BenchmarkResults"):
            match = pattern.match(benchmark.get("name", ""))
            mean = benchmark.find("mean")
            if match is None or mean is None:
                continue
            operation, width, size = match.groups()
            value = float(mean.get("value")) / 1e6
            low = float(mean.get("lowerBound", mean.get("value"))) / 1e6
            high = float(mean.get("upperBound", mean.get("value"))) / 1e6
            results.setdefault(operation, {}).setdefault(int(width), []).append(
                (int(size), value, low, high)
            )

    if not results:
        raise ValueError(f"No Veb scaling benchmarks found in {xml_path}")

    OUTPUTS.mkdir(parents=True, exist_ok=True)
    for operation, by_width in sorted(results.items()):
        fig, axis = plt.subplots(figsize=(9, 5.5))
        for index, (width, points) in enumerate(sorted(by_width.items())):
            points.sort(key=lambda point: point[0])
            sizes = [p[0] for p in points]
            means = [p[1] for p in points]
            errors = [
                [max(0, p[1] - p[2]) for p in points],
                [max(0, p[3] - p[1]) for p in points],
            ]
            axis.errorbar(
                sizes,
                means,
                yerr=errors,
                marker="o",
                capsize=2,
                linewidth=1.7,
                color=COLORS[index % len(COLORS)],
                label=f"W={width} bits",
            )
        axis.set_xscale("log")
        axis.set_xlabel("Number of elements (N)")
        style_axis(axis)
        axis.set_title(operation)
        axis.legend(title="Key width", frameon=False)
        fig.tight_layout()
        slug = re.sub(r"[^a-z0-9]+", "_", operation.casefold()).strip("_")
        output = OUTPUTS / f"veb_{slug}.png"
        fig.savefig(output, dpi=180, bbox_inches="tight")
        plt.close(fig)
        print(f"Saved {output}")
