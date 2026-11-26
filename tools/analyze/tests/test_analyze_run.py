import json
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

from PIL import Image

ANALYZE_SCRIPT = Path(__file__).resolve().parents[1] / "analyze_run.py"


def _epoch(year, month, day, hour, minute, second=0) -> int:
    return int(datetime(year, month, day, hour, minute, second, tzinfo=timezone.utc).timestamp())


def _make_photo(path: Path, boundary_row: int) -> None:
    img = Image.new("L", (100, 100), color=220)
    pixels = img.load()
    for y in range(boundary_row, 100):
        for x in range(100):
            pixels[x, y] = 40
    img.convert("RGB").save(path, "JPEG")


def _build_fake_dataset(root: Path) -> None:
    (root / "data").mkdir()
    photos_dir = root / "photos" / "2026-11-12"
    photos_dir.mkdir(parents=True)

    lines = []
    t = _epoch(2026, 11, 12, 9, 0)
    end = _epoch(2026, 11, 12, 11, 0)
    while t <= end:
        lines.append(f"{t},18.5,50.0")
        t += 60
    (root / "data" / "2026-11-12.csv").write_text("\n".join(lines) + "\n")

    (root / "data" / "boots.csv").write_text(f"{_epoch(2026, 11, 12, 9, 0)},0.7.0,power on\n")

    (root / "data" / "runs.csv").write_text(
        f"{_epoch(2026, 11, 12, 9, 0)},start,proef-test,zuurdesem,20.0,20.0,20.0,22.0,\n"
        f"{_epoch(2026, 11, 12, 11, 0)},end,proef-test,,,,,,klaar\n"
    )

    # One photo every 15 minutes, surface (boundary row) rising (moving
    # up the frame) by 2 px per photo so the rise curve is not flat.
    boundary = 80
    t = _epoch(2026, 11, 12, 9, 0)
    while t <= end:
        utc_time = datetime.fromtimestamp(t, tz=timezone.utc)
        name = f"{utc_time.hour:02d}{utc_time.minute:02d}{utc_time.second:02d}.jpg"
        _make_photo(photos_dir / name, boundary)
        boundary = max(10, boundary - 2)
        t += 900


def test_analyze_run_end_to_end_by_run_name(tmp_path):
    sdcard = tmp_path / "sdcard"
    sdcard.mkdir()
    _build_fake_dataset(sdcard)

    config = tmp_path / "proef-test.json"
    config.write_text(json.dumps({"crop": {"x": 0, "y": 0, "w": 100, "h": 100}, "px_per_cm": 10.0}))

    out_dir = tmp_path / "out"
    result = subprocess.run(
        [
            sys.executable,
            str(ANALYZE_SCRIPT),
            "--sdcard",
            str(sdcard),
            "--run",
            "proef-test",
            "--config",
            str(config),
            "--out",
            str(out_dir),
        ],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stderr

    assert (out_dir / "chart.png").is_file()
    assert (out_dir / "overview.jpg").is_file()
    assert (out_dir / "crops.jpg").is_file()
    assert (out_dir / "rise.png").is_file()

    summary = (out_dir / "summary.md").read_text()
    assert "proef-test" in summary
    assert "Eindstijging" in summary
    assert "Herstarts" in summary


def test_analyze_run_with_named_surface_regions(tmp_path):
    sdcard = tmp_path / "sdcard"
    sdcard.mkdir()
    _build_fake_dataset(sdcard)

    config = tmp_path / "proef-test.json"
    config.write_text(
        json.dumps(
            {
                "crop": {"x": 0, "y": 0, "w": 100, "h": 100},
                "px_per_cm": 10.0,
                "surface_regions": {
                    "back": {"x": 0, "y": 0, "w": 50, "h": 100, "smoothing": 3, "sign": "abs"},
                    "front": {"x": 50, "y": 0, "w": 50, "h": 100, "smoothing": 1, "sign": "falling"},
                },
            }
        )
    )

    out_dir = tmp_path / "out"
    result = subprocess.run(
        [
            sys.executable,
            str(ANALYZE_SCRIPT),
            "--sdcard",
            str(sdcard),
            "--run",
            "proef-test",
            "--config",
            str(config),
            "--out",
            str(out_dir),
        ],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stderr
    assert (out_dir / "rise.png").is_file()

    summary = (out_dir / "summary.md").read_text()
    assert "Eindstijging (back)" in summary
    assert "Eindstijging (front)" in summary


def test_analyze_run_without_config_skips_rise_curve(tmp_path):
    sdcard = tmp_path / "sdcard"
    sdcard.mkdir()
    _build_fake_dataset(sdcard)

    out_dir = tmp_path / "out"
    result = subprocess.run(
        [
            sys.executable,
            str(ANALYZE_SCRIPT),
            "--sdcard",
            str(sdcard),
            "--start",
            "2026-11-12 10:00",
            "--end",
            "2026-11-12 12:00",
            "--out",
            str(out_dir),
        ],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stderr
    assert (out_dir / "chart.png").is_file()
    assert (out_dir / "overview.jpg").is_file()
    assert not (out_dir / "crops.jpg").exists()
    summary = (out_dir / "summary.md").read_text()
    assert "niet berekend" in summary
