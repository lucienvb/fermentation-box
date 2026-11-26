#!/usr/bin/env python3
"""Analyzes one fermentation run from a copy of the SD card: a climate
chart, an hourly photo overview, jar crops, a rise curve in cm, and a
markdown summary. Runs on the laptop, not on the board - see
tools/analyze/requirements.txt for pinned dependencies and NOTES.md ("F2e")
for the design notes.

Usage:
    python3 analyze_run.py --sdcard /path/to/sdcard/copy --run proef-3 --out out/proef-3
    python3 analyze_run.py --sdcard /path/to/sdcard/copy \\
        --start "2026-11-12 09:35" --end "2026-11-12 22:20" --out out/proef-2 \\
        --config runs/proef-2.json

Config file (optional, enables the crops/rise curve output). "crop" is the
visual region shown in crops.jpg; "surface_regions" are one or more named
sub-regions tracked for the rise curve (ported from the two hardcoded
boxes - "front"/"back" edge of the dough - in tools/analyze/prototype/
rise.py). If "surface_regions" is omitted but "crop" + "px_per_cm" are
given, that single region is used as a fallback, named "surface":
    {
      "crop": {"x": 400, "y": 280, "w": 250, "h": 280},
      "px_per_cm": 60.0,
      "surface_regions": {
        "back":  {"x": 420, "y": 300, "w": 40, "h": 180, "smoothing": 7, "sign": "abs"},
        "front": {"x": 500, "y": 420, "w": 100, "h": 120, "smoothing": 5, "sign": "rising"}
      }
    }
"""

from __future__ import annotations

import argparse
import json
import statistics
from dataclasses import dataclass
from pathlib import Path

import matplotlib

matplotlib.use("Agg")  # No display available when run from the CLI/CI.
import matplotlib.dates as mdates
import matplotlib.pyplot as plt
import numpy as np
from PIL import Image, ImageDraw

from rise import rise_cm_series, surface_row
from sdcard_data import (
    list_photos,
    local_to_epoch,
    read_boots,
    read_measurements,
    resolve_run_range,
    to_local,
)

MEASUREMENT_GAP_S = 150  # ~2.5x the 1-minute measurement interval (main.cpp).
HOURLY_STEP_S = 3600
HOURLY_TOLERANCE_S = 600
THUMB_SIZE = (160, 120)
GRID_COLUMNS = 6


@dataclass
class CropRegion:
    x: int
    y: int
    w: int
    h: int


@dataclass
class SurfaceRegion:
    """One named sub-region tracked for the rise curve - see rise.py's
    surface_row() for what `smoothing`/`sign` do. Ported from the two
    hardcoded pixel boxes ("front"/"back") in tools/analyze/prototype/
    rise.py."""

    x: int
    y: int
    w: int
    h: int
    smoothing: int = 1
    sign: str = "abs"


def load_config(path: Path | None) -> tuple[CropRegion | None, float | None, dict[str, SurfaceRegion]]:
    if path is None:
        return None, None, {}

    data = json.loads(path.read_text())
    crop_data = data.get("crop")
    crop = CropRegion(**crop_data) if crop_data else None
    px_per_cm = data.get("px_per_cm")

    regions_data = data.get("surface_regions")
    if regions_data:
        regions = {name: SurfaceRegion(**fields) for name, fields in regions_data.items()}
    elif crop_data and px_per_cm:
        # No named regions given: fall back to tracking the single visual
        # crop itself, so a minimal config (just crop + px_per_cm) still
        # produces a rise curve.
        regions = {"surface": SurfaceRegion(**crop_data)}
    else:
        regions = {}

    return crop, px_per_cm, regions


def build_climate_chart(measurements, out_path: Path) -> None:
    fig, ax_temp = plt.subplots(figsize=(10, 4))
    ax_hum = ax_temp.twinx()

    times = [to_local(m.epoch) for m in measurements]
    temps = [m.temperature_c for m in measurements]
    hums = [m.humidity_pct for m in measurements]

    # Break the line at a measurement gap or a failed reading, so neither
    # shows up as a (misleadingly smooth) connecting line.
    temps_gapped: list[float] = []
    hums_gapped: list[float] = []
    for i, m in enumerate(measurements):
        gap = i > 0 and (m.epoch - measurements[i - 1].epoch) > MEASUREMENT_GAP_S
        temps_gapped.append(float("nan") if gap or m.temperature_c is None else m.temperature_c)
        hums_gapped.append(float("nan") if gap or m.humidity_pct is None else m.humidity_pct)

    ax_temp.plot(times, temps_gapped, color="tab:red", label="Temperatuur (°C)")
    ax_hum.plot(times, hums_gapped, color="tab:blue", label="Vochtigheid (%)")

    failed_times = [to_local(m.epoch) for m in measurements if m.temperature_c is None]
    if failed_times:
        ax_temp.scatter(
            failed_times,
            [ax_temp.get_ylim()[0]] * len(failed_times),
            marker="x",
            color="black",
            label="Mislukte meting",
            zorder=5,
        )

    ax_temp.set_ylabel("Temperatuur (°C)", color="tab:red")
    ax_hum.set_ylabel("Vochtigheid (%)", color="tab:blue")
    ax_temp.xaxis.set_major_formatter(mdates.DateFormatter("%d-%m %H:%M"))
    fig.autofmt_xdate()
    fig.legend(loc="upper center", ncol=3)
    fig.tight_layout()
    fig.savefig(out_path)
    plt.close(fig)


def _label_tile(img: Image.Image, text: str) -> Image.Image:
    tile = img.copy()
    draw = ImageDraw.Draw(tile)
    draw.rectangle([0, 0, tile.width, 14], fill=(0, 0, 0))
    draw.text((2, 1), text, fill=(255, 255, 255))
    return tile


def _grid_sheet(tiles: list[Image.Image]) -> Image.Image:
    rows = (len(tiles) + GRID_COLUMNS - 1) // GRID_COLUMNS
    sheet = Image.new("RGB", (THUMB_SIZE[0] * GRID_COLUMNS, THUMB_SIZE[1] * rows), (40, 40, 40))
    for i, tile in enumerate(tiles):
        x = (i % GRID_COLUMNS) * THUMB_SIZE[0]
        y = (i // GRID_COLUMNS) * THUMB_SIZE[1]
        sheet.paste(tile, (x, y))
    return sheet


def _hourly_photo_slots(photos: list[tuple[int, Path]], start_epoch: int, end_epoch: int):
    """One (epoch, path)-or-None per hour mark between start and end,
    picking the closest photo within HOURLY_TOLERANCE_S (gaps stay
    visible as a None slot rather than silently skipping the hour)."""
    slots = []
    hour = start_epoch - (start_epoch % HOURLY_STEP_S)
    while hour <= end_epoch:
        candidates = [p for p in photos if abs(p[0] - hour) <= HOURLY_TOLERANCE_S]
        best = min(candidates, key=lambda p: abs(p[0] - hour)) if candidates else None
        slots.append((hour, best))
        hour += HOURLY_STEP_S
    return slots


def build_hourly_overview(photos: list[tuple[int, Path]], start_epoch: int, end_epoch: int,
                           out_path: Path) -> None:
    tiles = []
    for hour, photo in _hourly_photo_slots(photos, start_epoch, end_epoch):
        label = to_local(hour).strftime("%d-%m %H:%M")
        if photo is None:
            placeholder = Image.new("RGB", THUMB_SIZE, (90, 30, 30))
        else:
            with Image.open(photo[1]) as img:
                placeholder = img.convert("RGB").resize(THUMB_SIZE)
        tiles.append(_label_tile(placeholder, label))
    if tiles:
        _grid_sheet(tiles).save(out_path)


def build_crops_overview(photos: list[tuple[int, Path]], crop: CropRegion, start_epoch: int,
                          end_epoch: int, out_path: Path) -> None:
    box = (crop.x, crop.y, crop.x + crop.w, crop.y + crop.h)
    tiles = []
    for hour, photo in _hourly_photo_slots(photos, start_epoch, end_epoch):
        label = to_local(hour).strftime("%d-%m %H:%M")
        if photo is None:
            placeholder = Image.new("RGB", THUMB_SIZE, (90, 30, 30))
        else:
            with Image.open(photo[1]) as img:
                placeholder = img.convert("RGB").crop(box).resize(THUMB_SIZE)
        tiles.append(_label_tile(placeholder, label))
    if tiles:
        _grid_sheet(tiles).save(out_path)


def compute_rise_series(photos: list[tuple[int, Path]], region: SurfaceRegion, px_per_cm: float):
    box = (region.x, region.y, region.x + region.w, region.y + region.h)
    rows = []
    for _epoch, path in photos:
        with Image.open(path) as img:
            gray = np.asarray(img.convert("L").crop(box), dtype=float)
        rows.append(surface_row(gray, smoothing=region.smoothing, sign=region.sign))
    return rise_cm_series(rows, px_per_cm)


def build_rise_chart(photos: list[tuple[int, Path]], rise_by_region: dict[str, list[float]],
                      out_path: Path) -> None:
    times = [to_local(epoch) for epoch, _path in photos]
    fig, ax = plt.subplots(figsize=(10, 3.5))
    for name in sorted(rise_by_region):
        ax.plot(times, rise_by_region[name], marker="o", markersize=3, label=name)
    ax.set_ylabel("Stijging (cm)")
    ax.legend(frameon=False)
    ax.xaxis.set_major_formatter(mdates.DateFormatter("%d-%m %H:%M"))
    fig.autofmt_xdate()
    fig.tight_layout()
    fig.savefig(out_path)
    plt.close(fig)


def detect_peak(photos: list[tuple[int, Path]], rise_cm: list[float]):
    """Returns the local time of the peak if the rise clearly comes back
    down afterwards (by more than 0.2 cm from the max), else None (still
    rising, flat, or too noisy to tell)."""
    if len(rise_cm) < 3:
        return None
    peak_i = int(np.argmax(rise_cm))
    if peak_i == len(rise_cm) - 1:
        return None
    if rise_cm[peak_i] - rise_cm[-1] <= 0.2:
        return None
    return to_local(photos[peak_i][0])


def build_summary(run_label: str, start_epoch: int, end_epoch: int, measurements, boots,
                   photos, rise_by_region: dict[str, list[float]], peak_by_region: dict,
                   out_path: Path) -> None:
    temps = [m.temperature_c for m in measurements if m.temperature_c is not None]
    hums = [m.humidity_pct for m in measurements if m.humidity_pct is not None]
    failed = sum(1 for m in measurements if m.temperature_c is None)

    gaps = []
    for prev, cur in zip(measurements, measurements[1:]):
        if cur.epoch - prev.epoch > MEASUREMENT_GAP_S:
            gaps.append((to_local(prev.epoch), to_local(cur.epoch)))

    lines = [
        f"# Analyse: {run_label}",
        "",
        f"- Periode: {to_local(start_epoch):%d-%m-%Y %H:%M} - {to_local(end_epoch):%d-%m-%Y %H:%M} (lokale tijd)",
        f"- Metingen: {len(measurements)} ({failed} mislukt)",
        f"- Foto's: {len(photos)}",
        f"- Herstarts tijdens de proef: {len(boots)}",
    ]
    for boot in boots:
        lines.append(f"  - {to_local(boot.epoch):%d-%m %H:%M} - {boot.firmware_version} - {boot.reset_reason}")
    lines.append(f"- Gaten in de metingen (> {MEASUREMENT_GAP_S}s): {len(gaps)}")
    for gap_start, gap_end in gaps:
        lines.append(f"  - {gap_start:%d-%m %H:%M} - {gap_end:%d-%m %H:%M}")
    if temps:
        lines.append(
            f"- Temperatuur: {min(temps):.1f}-{max(temps):.1f} °C (gem. {statistics.mean(temps):.1f})"
        )
    if hums:
        lines.append(
            f"- Vochtigheid: {min(hums):.1f}-{max(hums):.1f} % (gem. {statistics.mean(hums):.1f})"
        )
    if rise_by_region:
        for name in sorted(rise_by_region):
            series = rise_by_region[name]
            if not series:
                continue
            peak = peak_by_region.get(name)
            lines.append(f"- Eindstijging ({name}): {series[-1]:.2f} cm")
            lines.append(
                f"  - Piek: {peak:%d-%m %H:%M}" if peak else "  - Piek: geen duidelijke piek (nog stijgend of vlak)"
            )
    else:
        lines.append(
            "- Stijgingscurve: niet berekend (geen config met crop/px_per_cm of surface_regions opgegeven)"
        )

    out_path.write_text("\n".join(lines) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--sdcard", required=True, type=Path, help="Directory with a copy of the SD card")
    parser.add_argument("--run", help="Run name, looked up in /data/runs.csv")
    parser.add_argument("--start", help='Local start time, "YYYY-MM-DD HH:MM" (Europe/Amsterdam)')
    parser.add_argument("--end", help='Local end time, "YYYY-MM-DD HH:MM" (Europe/Amsterdam)')
    parser.add_argument("--config", type=Path, help="Optional JSON file with crop region + px_per_cm")
    parser.add_argument("--out", required=True, type=Path, help="Output directory for this run")
    args = parser.parse_args()

    if args.run:
        start_epoch, end_epoch = resolve_run_range(args.sdcard, args.run)
        run_label = args.run
    elif args.start and args.end:
        start_epoch, end_epoch = local_to_epoch(args.start), local_to_epoch(args.end)
        run_label = f"{args.start} - {args.end}"
    else:
        parser.error("Pass either --run or both --start and --end")
        return

    measurements = read_measurements(args.sdcard, start_epoch, start_epoch if end_epoch is None else end_epoch)
    photos = list_photos(args.sdcard, start_epoch, end_epoch or start_epoch)
    if end_epoch is None:
        # Run has no "end" event yet (F2e, see NOTES.md): fall back to the
        # last available data point so the output still covers everything.
        last_epochs = [m.epoch for m in measurements] + [p[0] for p in photos]
        end_epoch = max(last_epochs) if last_epochs else start_epoch
        measurements = read_measurements(args.sdcard, start_epoch, end_epoch)
        photos = list_photos(args.sdcard, start_epoch, end_epoch)
    boots = read_boots(args.sdcard, start_epoch, end_epoch)

    args.out.mkdir(parents=True, exist_ok=True)
    build_climate_chart(measurements, args.out / "chart.png")
    build_hourly_overview(photos, start_epoch, end_epoch, args.out / "overview.jpg")

    crop, px_per_cm, regions = load_config(args.config)
    if crop:
        build_crops_overview(photos, crop, start_epoch, end_epoch, args.out / "crops.jpg")

    rise_by_region: dict[str, list[float]] = {}
    peak_by_region: dict[str, object] = {}
    if regions and px_per_cm:
        for name, region in regions.items():
            series = compute_rise_series(photos, region, px_per_cm)
            rise_by_region[name] = series
            peak_by_region[name] = detect_peak(photos, series)
        build_rise_chart(photos, rise_by_region, args.out / "rise.png")

    build_summary(run_label, start_epoch, end_epoch, measurements, boots, photos, rise_by_region,
                  peak_by_region, args.out / "summary.md")
    print(f"Written to {args.out}")


if __name__ == "__main__":
    main()
