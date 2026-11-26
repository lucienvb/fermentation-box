import sys
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from sdcard_data import (  # noqa: E402
    list_photos,
    local_to_epoch,
    read_boots,
    read_measurements,
    read_runs,
    resolve_run_range,
    to_local,
)


def _epoch(year, month, day, hour, minute, second=0) -> int:
    return int(datetime(year, month, day, hour, minute, second, tzinfo=timezone.utc).timestamp())


def _write_fake_sdcard(root: Path) -> None:
    (root / "data").mkdir()
    (root / "photos" / "2026-11-12").mkdir(parents=True)

    day_csv = root / "data" / "2026-11-12.csv"
    day_csv.write_text(
        f"{_epoch(2026, 11, 12, 9, 0)},18.5,50.0\n"
        f"{_epoch(2026, 11, 12, 9, 1)},18.6,50.1\n"
        f"{_epoch(2026, 11, 12, 9, 2)},,\n"  # failed reading
        f"{_epoch(2026, 11, 12, 12, 0)},19.0,51.0\n"  # far outside any 1-min gap
    )

    (root / "data" / "boots.csv").write_text(f"{_epoch(2026, 11, 12, 9, 0)},0.7.0,power on\n")

    (root / "data" / "runs.csv").write_text(
        f'{_epoch(2026, 11, 12, 9, 35)},start,proef-2,zuurdesem,20.0,20.0,20.0,22.0,"lauw, kort"\n'
        f"{_epoch(2026, 11, 12, 22, 20)},end,proef-2,,,,,,klaar\n"
    )

    for hhmmss in ("090000", "100000"):
        (root / "photos" / "2026-11-12" / f"{hhmmss}.jpg").write_bytes(b"\xff\xd8fake")


def test_read_measurements_filters_by_range_and_parses_empty_fields(tmp_path):
    _write_fake_sdcard(tmp_path)
    start = _epoch(2026, 11, 12, 9, 0)
    end = _epoch(2026, 11, 12, 9, 1)
    measurements = read_measurements(tmp_path, start, end)
    assert [m.epoch for m in measurements] == [start, end]
    assert measurements[0].temperature_c == 18.5

    all_measurements = read_measurements(tmp_path, start, _epoch(2026, 11, 12, 12, 0))
    failed = [m for m in all_measurements if m.temperature_c is None]
    assert len(failed) == 1


def test_read_boots(tmp_path):
    _write_fake_sdcard(tmp_path)
    boots = read_boots(tmp_path, _epoch(2026, 11, 12, 0, 0), _epoch(2026, 11, 12, 23, 0))
    assert len(boots) == 1
    assert boots[0].firmware_version == "0.7.0"


def test_read_runs_handles_quoted_comma_in_note(tmp_path):
    _write_fake_sdcard(tmp_path)
    runs = read_runs(tmp_path)
    start_event = next(r for r in runs if r.event == "start")
    assert start_event.note == "lauw, kort"
    assert start_event.starter_g == 20.0


def test_resolve_run_range_finds_start_and_end(tmp_path):
    _write_fake_sdcard(tmp_path)
    start_epoch, end_epoch = resolve_run_range(tmp_path, "proef-2")
    assert start_epoch == _epoch(2026, 11, 12, 9, 35)
    assert end_epoch == _epoch(2026, 11, 12, 22, 20)


def test_resolve_run_range_missing_run_raises(tmp_path):
    _write_fake_sdcard(tmp_path)
    try:
        resolve_run_range(tmp_path, "does-not-exist")
        assert False, "expected ValueError"
    except ValueError:
        pass


def test_list_photos_parses_epoch_from_path(tmp_path):
    _write_fake_sdcard(tmp_path)
    photos = list_photos(tmp_path, _epoch(2026, 11, 12, 0, 0), _epoch(2026, 11, 12, 23, 0))
    assert [epoch for epoch, _path in photos] == [
        _epoch(2026, 11, 12, 9, 0),
        _epoch(2026, 11, 12, 10, 0),
    ]


def test_local_to_epoch_and_back_roundtrip():
    epoch = local_to_epoch("2026-11-12 09:35")
    local = to_local(epoch)
    assert (local.hour, local.minute) == (9, 35)
