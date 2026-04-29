#!/usr/bin/env python3

import re
import statistics
import subprocess
from pathlib import Path

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
import numpy as np
from PIL import Image


ROOT = Path(__file__).resolve().parent.parent
INPUT_DIR = ROOT / "input"
BENCHMARK_DIR = ROOT / "benchmarks"
PROCESSED_DIR = BENCHMARK_DIR / "processed_outputs"
APP = ROOT / "build" / "bin" / "app"

RUNS = 10
MODES = ["seq", "rows", "cols", "pixels", "grid"]

TIME_PATTERN = re.compile(r"processing time:\s*([0-9]+(?:\.[0-9]+)?)\s*ms")

CASES = [
    ("gauss_5x5", "gauss 5x5", [("gauss", 5, 5, None)]),
    ("mean_5x5", "mean 5x5", [("mean", 5, 5, None)]),
    ("motion_9x9", "motion 9x9", [("motion", 9, 9, None)]),
    ("sharpen_5x5", "sharpen 5x5", [("sharpen", 5, 5, None)]),
    ("edge_diagonal_5x5", "edge diagonal 5x5", [("edge", 5, 5, "diagonal")]),
    (
        "gauss_5x5_then_sharpen_3x3",
        "gauss 5x5 -> sharpen 3x3",
        [("gauss", 5, 5, None), ("sharpen", 3, 3, None)],
    ),
    (
        "motion_9x9_then_edge_omni_3x3",
        "motion 9x9 -> edge omni 3x3",
        [("motion", 9, 9, None), ("edge", 3, 3, "omni")],
    ),
]

MODE_NAMES = {
    "seq": "Sequential",
    "rows": "Parallel rows",
    "cols": "Parallel cols",
    "pixels": "Parallel pixels",
    "grid": "Parallel grid",
}

MODE_COLORS = {
    "seq": "#BC3C28",
    "rows": "#0072B5",
    "cols": "#2E8B57",
    "pixels": "#8A52A1",
    "grid": "#C58B00",
}


def get_images():
    images = []

    for path in sorted(INPUT_DIR.iterdir()):
        if path.suffix.lower() not in {".jpg", ".jpeg", ".png"}:
            continue

        with Image.open(path) as image:
            width, height = image.size

        images.append(
            {
                "path": path,
                "name": path.name,
                "stem": path.stem,
                "width": width,
                "height": height,
                "megapixels": width * height / 1_000_000,
            }
        )

    return images


def build_command(image, filters, mode, output_path):
    command = [str(APP), "-i", str(image["path"]), "-o", str(output_path)]

    for name, height, width, filter_type in filters:
        command += ["-f", name, "-h", str(height), "-w", str(width)]
        if filter_type:
            command += ["-t", filter_type]

    if mode == "seq":
        command += ["-s"]
    else:
        command += ["-p", mode]

    return command


def run_program(command):
    result = subprocess.run(command, check=True, capture_output=True, text=True)
    return float(TIME_PATTERN.search(result.stdout).group(1))


def run_benchmark(image, case_name, filters):
    results = {}

    for mode in MODES:
        values = []

        for run_number in range(1, RUNS + 1):
            output_path = PROCESSED_DIR / f"{image['stem']}_{case_name}_{mode}_{run_number}.png"
            values.append(run_program(build_command(image, filters, mode, output_path)))

        results[mode] = {
            "values": values,
            "mean": statistics.fmean(values),
            "min": min(values),
            "max": max(values),
        }

    seq_mean = results["seq"]["mean"]
    for mode in MODES:
        results[mode]["speedup"] = seq_mean / results[mode]["mean"]

    return results


def draw_plot(image, case_name, case_title, results):
    fig, ax = plt.subplots(figsize=(16, 10), dpi=180)
    fig.patch.set_facecolor("#f6f3ee")
    ax.set_facecolor("#fffdfa")

    all_values = [value for mode in MODES for value in results[mode]["values"]]
    min_time = min(all_values)
    max_time = max(all_values)
    padding = max(max_time - min_time, max_time * 0.15, 5.0)
    ax.set_ylim(max(0, min_time - padding * 0.18), max_time + padding * 0.42)

    for x, mode in enumerate(MODES):
        values = results[mode]["values"]
        mean = results[mode]["mean"]
        color = MODE_COLORS[mode]
        jitter = np.linspace(-0.18, 0.18, len(values))

        ax.scatter(
            x + jitter,
            values,
            s=95,
            color=color,
            alpha=0.78,
            edgecolors="white",
            linewidths=1.2,
            zorder=3,
        )
        ax.vlines(x, results[mode]["min"], results[mode]["max"], color=color, alpha=0.35, linewidth=3)
        ax.scatter([x], [mean], marker="D", s=185, color="#111111", edgecolors="white", linewidths=1.6)
        ax.hlines(mean, x - 0.22, x + 0.22, color="#111111", linewidth=2.6)

        ax.text(
            x,
            results[mode]["max"] + padding * 0.06,
            f"mean {mean:.1f} ms\n{results[mode]['speedup']:.2f}x vs seq",
            ha="center",
            va="bottom",
            fontsize=13,
            fontweight="bold",
        )

    ax.grid(axis="y", color="#d9d2c7", linewidth=1.0, alpha=0.85)
    ax.set_axisbelow(True)
    ax.set_xticks(range(len(MODES)))
    ax.set_xticklabels([MODE_NAMES[mode] for mode in MODES], fontsize=14, fontweight="bold")
    ax.set_ylabel("Processing time, ms", fontsize=16, fontweight="bold")
    ax.set_xlabel("Execution mode", fontsize=16, fontweight="bold")
    ax.tick_params(axis="y", labelsize=13)

    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.spines["left"].set_color("#7a746b")
    ax.spines["bottom"].set_color("#7a746b")
    ax.axhline(results["seq"]["mean"], color=MODE_COLORS["seq"], linestyle="--", linewidth=1.8, alpha=0.55)

    fig.suptitle(
        f"{image['name']} | {case_name}\n"
        f"{image['width']}x{image['height']} px | {image['megapixels']:.2f} MP | {case_title}",
        fontsize=20,
        fontweight="bold",
        y=0.97,
    )
    ax.set_title("10 launch points per mode", fontsize=14, pad=18)

    fig.tight_layout(rect=(0, 0, 1, 0.93))
    fig.savefig(BENCHMARK_DIR / f"{image['stem']}_{case_name}_benchmark.png", bbox_inches="tight")
    plt.close(fig)


def main():
    BENCHMARK_DIR.mkdir(exist_ok=True)
    PROCESSED_DIR.mkdir(exist_ok=True)

    for image in get_images():
        for case_name, case_title, filters in CASES:
            print(f"{image['name']} | {case_name}")
            results = run_benchmark(image, case_name, filters)
            draw_plot(image, case_name, case_title, results)


if __name__ == "__main__":
    main()
