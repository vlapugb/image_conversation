#!/usr/bin/env python3

import json
import os
import platform
import re
import statistics
import subprocess
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp/image_conversation_matplotlib")

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
import numpy as np
from PIL import Image


ROOT = Path(__file__).resolve().parent.parent
INPUT_DIR = ROOT / "input"
BENCHMARK_DIR = ROOT / "benchmarks"
OUTPUT_DIR = Path("/tmp/image_conversation_benchmark_outputs")
APP = ROOT / "build" / "bin" / "app"

RUNS = 10
MODES = ["seq", "rows", "cols", "pixels", "grid"]
IMAGE_EXTENSIONS = {".jpg", ".jpeg", ".png"}
TIME_PATTERN = re.compile(r"processing time:\s*([0-9]+(?:\.[0-9]+)?)\s*ms")

CASE_NAME = "gauss_5x5_then_sharpen_3x3"
CASE_TITLE = "gauss 5x5 -> sharpen 3x3"
FILTERS = [
    ("gauss", 5, 5, None),
    ("sharpen", 3, 3, None),
]

SINGLE_IMAGES = [
    "satoru.jpg",
    "sunshine.jpg",
    "stariy_bog.png",
    "musashi.jpg",
    "sea.png",
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


def available_thread_count():
    if hasattr(os, "sched_getaffinity"):
        return len(os.sched_getaffinity(0))

    return os.cpu_count() or 1


def benchmark_env():
    env = os.environ.copy()
    env.setdefault("OMP_NUM_THREADS", str(available_thread_count()))
    return env


def image_info(path):
    with Image.open(path) as image:
        width, height = image.size

    return {
        "name": path.name,
        "path": str(path.relative_to(ROOT)),
        "stem": path.stem,
        "width": width,
        "height": height,
        "megapixels": width * height / 1_000_000,
    }


def get_single_images():
    return [image_info(INPUT_DIR / name) for name in SINGLE_IMAGES]


def get_pipeline_images():
    images = []

    for path in sorted(INPUT_DIR.iterdir()):
        if path.suffix.lower() not in IMAGE_EXTENSIONS:
            continue
        if "3840x2160" not in path.name:
            continue

        info = image_info(path)
        if info["width"] >= 3840 and info["height"] >= 2160:
            images.append(info)

    if len(images) < 10:
        raise RuntimeError("pipeline benchmark needs at least ten 4K input images")

    return images[:10]


def add_filter_args(command):
    for name, height, width, filter_type in FILTERS:
        command += ["-f", name, "-h", str(height), "-w", str(width)]
        if filter_type is not None:
            command += ["-t", filter_type]


def add_mode_args(command, mode):
    if mode == "seq":
        command += ["-s"]
    else:
        command += ["-p", mode]


def build_single_command(image, mode):
    output_path = OUTPUT_DIR / "single" / f"{image['stem']}_{mode}.png"
    command = [
        str(APP),
        "-i",
        str(ROOT / image["path"]),
        "-o",
        str(output_path),
    ]
    add_filter_args(command)
    add_mode_args(command, mode)
    return command


def build_pipeline_command(images, mode):
    command = [str(APP)]

    for index, image in enumerate(images, start=1):
        output_path = OUTPUT_DIR / "pipeline" / f"image_{index:02d}_{mode}.png"
        command += ["-i", str(ROOT / image["path"]), "-o", str(output_path)]

    add_filter_args(command)
    add_mode_args(command, mode)
    return command


def run_program(command, env):
    completed = subprocess.run(command, check=True, capture_output=True, text=True, env=env)
    match = TIME_PATTERN.search(completed.stdout)
    if match is None:
        raise RuntimeError(f"application output does not contain processing time: {completed.stdout}")

    return float(match.group(1))


def summarize_runs(values, seq_mean):
    mean = statistics.fmean(values)
    return {
        "runs_ms": values,
        "mean_ms": mean,
        "min_ms": min(values),
        "max_ms": max(values),
        "speedup_vs_seq": seq_mean / mean,
    }


def run_mode_benchmark(command_builder, env, progress_prefix):
    raw_results = {}

    for mode in MODES:
        values = []

        for run_number in range(1, RUNS + 1):
            value = run_program(command_builder(mode), env)
            values.append(value)
            print(f"{progress_prefix} | {mode} | run {run_number}/{RUNS}: {value:.3f} ms", flush=True)

        raw_results[mode] = values

    seq_mean = statistics.fmean(raw_results["seq"])
    return {mode: summarize_runs(values, seq_mean) for mode, values in raw_results.items()}


def draw_plot(title, subtitle, output_path, results):
    fig, ax = plt.subplots(figsize=(16, 10), dpi=180)
    fig.patch.set_facecolor("#f6f3ee")
    ax.set_facecolor("#fffdfa")

    all_values = [value for mode in MODES for value in results[mode]["runs_ms"]]
    min_time = min(all_values)
    max_time = max(all_values)
    padding = max(max_time - min_time, max_time * 0.15, 5.0)
    ax.set_ylim(max(0.0, min_time - padding * 0.18), max_time + padding * 0.42)

    for x, mode in enumerate(MODES):
        values = results[mode]["runs_ms"]
        mean_time = results[mode]["mean_ms"]
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
            results[mode]["min_ms"],
            results[mode]["max_ms"],
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
            results[mode]["max_ms"] + padding * 0.06,
            f"mean {mean_time:.1f} ms\n{results[mode]['speedup_vs_seq']:.2f}x vs seq",
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
    ax.set_xlabel("Convolution runner", fontsize=16, fontweight="bold")
    ax.tick_params(axis="y", labelsize=13)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.spines["left"].set_color("#7a746b")
    ax.spines["bottom"].set_color("#7a746b")
    ax.axhline(
        results["seq"]["mean_ms"],
        color=MODE_COLORS["seq"],
        linestyle="--",
        linewidth=1.8,
        alpha=0.55,
    )

    fig.suptitle(title, fontsize=20, fontweight="bold", y=0.97)
    ax.set_title(subtitle, fontsize=14, pad=18)

    fig.tight_layout(rect=(0, 0, 1, 0.93))
    fig.savefig(output_path, bbox_inches="tight")
    plt.close(fig)


def run_single_benchmarks(env):
    results = []

    for image in get_single_images():
        benchmark = run_mode_benchmark(
            lambda mode, current=image: build_single_command(current, mode),
            env,
            f"single {image['name']}",
        )
        plot_path = BENCHMARK_DIR / f"readme_single_{image['stem']}_{CASE_NAME}_benchmark.png"
        draw_plot(
            f"{image['name']} | {image['width']}x{image['height']} px | {CASE_TITLE}",
            f"{RUNS} launches per runner",
            plot_path,
            benchmark,
        )
        results.append({"image": image, "results": benchmark, "plot_path": str(plot_path.relative_to(ROOT))})

    return results


def run_pipeline_benchmark(env):
    images = get_pipeline_images()
    benchmark = run_mode_benchmark(
        lambda mode: build_pipeline_command(images, mode),
        env,
        "pipeline 10x4K",
    )
    plot_path = BENCHMARK_DIR / f"readme_pipeline_{CASE_NAME}_benchmark.png"
    draw_plot(
        f"Pipeline | 10 images | {CASE_TITLE}",
        f"{RUNS} launches per runner",
        plot_path,
        benchmark,
    )

    return {"images": images, "results": benchmark, "plot_path": str(plot_path.relative_to(ROOT))}


def write_results(data):
    output_path = BENCHMARK_DIR / "readme_benchmark_results.json"
    with output_path.open("w", encoding="utf-8") as file:
        json.dump(data, file, indent=2)
        file.write("\n")

    return output_path


def main():
    if not APP.exists():
        raise RuntimeError("build/bin/app does not exist; build the app before benchmarking")

    BENCHMARK_DIR.mkdir(exist_ok=True)
    (OUTPUT_DIR / "single").mkdir(parents=True, exist_ok=True)
    (OUTPUT_DIR / "pipeline").mkdir(parents=True, exist_ok=True)

    env = benchmark_env()
    data = {
        "case": CASE_TITLE,
        "runs": RUNS,
        "modes": MODES,
        "mode_names": MODE_NAMES,
        "environment": {
            "system": platform.platform(),
            "machine": platform.machine(),
            "omp_num_threads": env["OMP_NUM_THREADS"],
        },
        "single_images": run_single_benchmarks(env),
        "pipeline": run_pipeline_benchmark(env),
    }

    output_path = write_results(data)
    print(f"results: {output_path.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
