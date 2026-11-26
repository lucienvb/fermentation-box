"""Reads the fixed file layout of an SD card copy (see CLAUDE.md /
NOTES.md): /data/YYYY-MM-DD.csv (measurements), /data/boots.csv,
/data/runs.csv (F2e) and /photos/YYYY-MM-DD/HHMMSS.jpg. Pure parsing, no
plotting - covered by tests/test_sdcard_data.py with a small fake dataset.
"""

from __future__ import annotations

import csv
import re
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from zoneinfo import ZoneInfo

LOCAL_TZ = ZoneInfo("Europe/Amsterdam")

_DAY_CSV_RE = re.compile(r"^\d{4}-\d{2}-\d{2}\.csv$")
_PHOTO_RE = re.compile(r"^(\d{2})(\d{2})(\d{2})\.jpg$")
_PHOTO_DIR_RE = re.compile(r"^(\d{4})-(\d{2})-(\d{2})$")


@dataclass
class Measurement:
    epoch: int
    temperature_c: float | None  # None for a failed reading ("epoch,,")
    humidity_pct: float | None


@dataclass
class BootRecord:
    epoch: int
    firmware_version: str
    reset_reason: str


@dataclass
class RunEvent:
    epoch: int
    event: str  # "start" or "end"
    name: str
    product: str
    starter_g: float | None
    flour_g: float | None
    water_g: float | None
    water_temp_c: float | None
    note: str


def to_local(epoch: int) -> datetime:
    """UTC epoch -> aware datetime in Europe/Amsterdam (local time, see
    CLAUDE.md: the board measures in UTC, charts read in local time)."""
    return datetime.fromtimestamp(epoch, tz=timezone.utc).astimezone(LOCAL_TZ)


def local_to_epoch(text: str) -> int:
    """"YYYY-MM-DD HH:MM" (or with seconds), interpreted as Europe/
    Amsterdam local time (handling the DST offset for that date) -> UTC
    epoch, for the --start/--end CLI arguments."""
    for fmt in ("%Y-%m-%d %H:%M:%S", "%Y-%m-%d %H:%M"):
        try:
            naive = datetime.strptime(text, fmt)
            break
        except ValueError:
            continue
    else:
        raise ValueError(f"Could not parse local time: {text!r}")
    return int(naive.replace(tzinfo=LOCAL_TZ).timestamp())


def _parse_float(field: str) -> float | None:
    return float(field) if field != "" else None


def read_measurements(sdcard_dir: Path, start_epoch: int, end_epoch: int) -> list[Measurement]:
    data_dir = sdcard_dir / "data"
    if not data_dir.is_dir():
        return []

    out: list[Measurement] = []
    for path in sorted(data_dir.iterdir()):
        if not _DAY_CSV_RE.match(path.name):
            continue
        with path.open(newline="") as f:
            for row in csv.reader(f):
                if len(row) != 3:
                    continue
                epoch = int(row[0])
                if not (start_epoch <= epoch <= end_epoch):
                    continue
                out.append(Measurement(epoch, _parse_float(row[1]), _parse_float(row[2])))
    out.sort(key=lambda m: m.epoch)
    return out


def read_boots(sdcard_dir: Path, start_epoch: int, end_epoch: int) -> list[BootRecord]:
    path = sdcard_dir / "data" / "boots.csv"
    if not path.is_file():
        return []

    out: list[BootRecord] = []
    with path.open(newline="") as f:
        for row in csv.reader(f):
            if len(row) != 3:
                continue
            epoch = int(row[0])
            if start_epoch <= epoch <= end_epoch:
                out.append(BootRecord(epoch, row[1], row[2]))
    out.sort(key=lambda b: b.epoch)
    return out


def read_runs(sdcard_dir: Path) -> list[RunEvent]:
    """All run events, unfiltered - resolve_run_range() below picks the
    ones for a given run name."""
    path = sdcard_dir / "data" / "runs.csv"
    if not path.is_file():
        return []

    out: list[RunEvent] = []
    with path.open(newline="") as f:
        for row in csv.reader(f):
            if len(row) != 9:
                continue
            epoch, event, name, product, starter_g, flour_g, water_g, water_temp_c, note = row
            out.append(
                RunEvent(
                    int(epoch),
                    event,
                    name,
                    product,
                    _parse_float(starter_g),
                    _parse_float(flour_g),
                    _parse_float(water_g),
                    _parse_float(water_temp_c),
                    note,
                )
            )
    out.sort(key=lambda r: r.epoch)
    return out


def resolve_run_range(sdcard_dir: Path, run_name: str) -> tuple[int, int | None]:
    """First "start" event for run_name and the first matching "end"
    after it. Returns (start_epoch, None) if the run has no end event yet
    (still running, or the SD card was copied mid-run) - callers should
    then fall back to the last available measurement/photo as the end.
    """
    runs = [r for r in read_runs(sdcard_dir) if r.name == run_name]
    starts = [r for r in runs if r.event == "start"]
    if not starts:
        raise ValueError(f"No start event found for run {run_name!r} in runs.csv")
    start = starts[0]
    ends = [r for r in runs if r.event == "end" and r.epoch >= start.epoch]
    return start.epoch, (ends[0].epoch if ends else None)


def list_photos(sdcard_dir: Path, start_epoch: int, end_epoch: int) -> list[tuple[int, Path]]:
    photos_dir = sdcard_dir / "photos"
    if not photos_dir.is_dir():
        return []

    out: list[tuple[int, Path]] = []
    for day_dir in sorted(photos_dir.iterdir()):
        match = _PHOTO_DIR_RE.match(day_dir.name)
        if not match or not day_dir.is_dir():
            continue
        year, month, day = (int(g) for g in match.groups())
        for path in sorted(day_dir.iterdir()):
            photo_match = _PHOTO_RE.match(path.name)
            if not photo_match:
                continue
            hour, minute, second = (int(g) for g in photo_match.groups())
            epoch = int(
                datetime(year, month, day, hour, minute, second, tzinfo=timezone.utc).timestamp()
            )
            if start_epoch <= epoch <= end_epoch:
                out.append((epoch, path))
    out.sort(key=lambda item: item[0])
    return out
