#!/usr/bin/env python3

import re
import statistics
import subprocess
from pathlib import Path

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
import numpy as np


ROOT = Path(__file__).resolve().parent.parent
INPUT_DIR = ROOT / "input"
BENCHMARK_DIR = ROOT / "benchmarks"
PROCESSED_DIR = BENCHMARK_DIR / "pipeline_outputs"
APP = ROOT / "build" / "bin" / "app"

RUNS = 10
MODES = ["seq", "rows", "cols", "pixels", "grid"]
IMAGE_EXTENSIONS = {".jpg", ".jpeg", ".png"}
TIME_PATTERN = re.compile(r"processing time:\s*([0-9]+(?:\.[0-9]+)?)\s*ms")
MIN_4K_WIDTH = 3840
MIN_4K_HEIGHT = 2160

CASE_NAME = "gauss_5x5_then_sharpen_3x3"
CASE_TITLE = "gauss 5x5 -> sharpen 3x3"
FILTERS = [
    ("gauss", 5, 5, None),
    ("sharpen", 3, 3, None),
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


def get_input_images():
    images = []

    for path in sorted(INPUT_DIR.iterdir()):
        if path.suffix.lower() not in IMAGE_EXTENSIONS:
            continue
        if "3840x2160" not in path.name:
            continue

        from PIL import Image

        with Image.open(path) as image:
            width, height = image.size

        if width >= MIN_4K_WIDTH and height >= MIN_4K_HEIGHT:
            images.append(path)

    return images[:10]


def build_command(images, mode, run_number):
    command = [str(APP)]

    for image_index, image_path in enumerate(images, start=1):
        output_name = f"image_{image_index:02d}_{CASE_NAME}_pipeline_{mode}.png"
        command += ["-i", str(image_path), "-o", str(PROCESSED_DIR / output_name)]

    for filter_name, height, width, filter_type in FILTERS:
        command += ["-f", filter_name, "-h", str(height), "-w", str(width)]
        if filter_type is not None:
            command += ["-t", filter_type]

    if mode == "seq":
        command += ["-s"]
    else:
        command += ["-p", mode]

    return command


def run_program(command):
    completed = subprocess.run(command, check=True, capture_output=True, text=True)
    match = TIME_PATTERN.search(completed.stdout)
    if match is None:
        raise RuntimeError("application output does not contain processing time")

    return float(match.group(1))


def run_benchmark(images):
    results = {}

    for mode in MODES:
        values = []
        for run_number in range(1, RUNS + 1):
            command = build_command(images, mode, run_number)
            values.append(run_program(command))

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


def draw_plot(results, image_count):
    fig, ax = plt.subplots(figsize=(16, 10), dpi=180)
    fig.patch.set_facecolor("#f6f3ee")
    ax.set_facecolor("#fffdfa")

    all_values = [value for mode in MODES for value in results[mode]["values"]]
    min_time = min(all_values)
    max_time = max(all_values)
    padding = max(max_time - min_time, max_time * 0.15, 5.0)
    ax.set_ylim(max(0.0, min_time - padding * 0.18), max_time + padding * 0.42)

    for x, mode in enumerate(MODES):
        values = results[mode]["values"]
        mean_time = results[mode]["mean"]
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
        ax.vlines(
            x,
            results[mode]["min"],
            results[mode]["max"],
            color=color,
            alpha=0.35,
            linewidth=3,
        )
        ax.scatter(
            [x],
            [mean_time],
            marker="D",
            s=185,
            color="#111111",
            edgecolors="white",
            linewidths=1.6,
        )
        ax.hlines(mean_time, x - 0.22, x + 0.22, color="#111111", linewidth=2.6)

        ax.text(
            x,
            results[mode]["max"] + padding * 0.06,
            f"mean {mean_time:.1f} ms\n{results[mode]['speedup']:.2f}x vs seq",
            ha="center",
            va="bottom",
            fontsize=13,
            fontweight="bold",
        )

    ax.grid(axis="y", color="#d9d2c7", linewidth=1.0, alpha=0.85)
    ax.set_axisbelow(True)
    ax.set_xticks(range(len(MODES)))
    ax.set_xticklabels([MODE_NAMES[mode] for mode in MODES], fontsize=14, fontweight="bold")
    ax.set_ylabel("Pipeline processing time, ms", fontsize=16, fontweight="bold")
    ax.set_xlabel("Convolution runner", fontsize=16, fontweight="bold")
    ax.tick_params(axis="y", labelsize=13)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.spines["left"].set_color("#7a746b")
    ax.spines["bottom"].set_color("#7a746b")
    ax.axhline(
        results["seq"]["mean"],
        color=MODE_COLORS["seq"],
        linestyle="--",
        linewidth=1.8,
        alpha=0.55,
    )

    fig.suptitle(
        f"Thread-pool image pipeline | {image_count} images | {CASE_TITLE}",
        fontsize=20,
        fontweight="bold",
        y=0.97,
    )
    ax.set_title(f"{RUNS} launches per runner", fontsize=14, pad=18)

    output_path = BENCHMARK_DIR / f"pipeline_{CASE_NAME}_convolution_runner_benchmark.png"
    fig.tight_layout(rect=(0, 0, 1, 0.93))
    fig.savefig(output_path, bbox_inches="tight")
    plt.close(fig)
    return output_path


def main():
    BENCHMARK_DIR.mkdir(exist_ok=True)
    PROCESSED_DIR.mkdir(exist_ok=True)

    images = get_input_images()
    if len(images) <= 1:
        raise RuntimeError("pipeline benchmark needs more than one 4K input image")

    print("pipeline images:", ", ".join(path.name for path in images))
    results = run_benchmark(images)
    plot_path = draw_plot(results, len(images))

    for mode in MODES:
        data = results[mode]
        print(
            f"{mode}: mean={data['mean']:.3f} ms, "
            f"min={data['min']:.3f} ms, max={data['max']:.3f} ms, "
            f"speedup={data['speedup']:.2f}x"
        )

    print(f"plot: {plot_path}")


if __name__ == "__main__":
    main()
