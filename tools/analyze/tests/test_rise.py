import sys
from pathlib import Path

import numpy as np
import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from rise import rise_cm_series, surface_row  # noqa: E402


def test_surface_row_finds_strongest_edge():
    gray = np.array(
        [
            [200, 200],
            [200, 200],
            [50, 50],
            [50, 50],
            [50, 50],
        ],
        dtype=float,
    )
    assert surface_row(gray) == 1


def test_surface_row_requires_at_least_two_rows():
    with pytest.raises(ValueError):
        surface_row(np.array([[1, 2, 3]], dtype=float))


def test_rise_cm_series_empty_input():
    assert rise_cm_series([], px_per_cm=10) == []


def test_rise_cm_series_rejects_non_positive_px_per_cm():
    with pytest.raises(ValueError):
        rise_cm_series([10, 9, 8], px_per_cm=0)


def test_rise_cm_series_baseline_is_zero():
    rise = rise_cm_series([100, 100, 100], px_per_cm=10)
    assert rise[0] == pytest.approx(0.0)


def test_rise_cm_series_decreasing_row_means_rising():
    # Row index decreases over time (surface moves up) -> rise increases.
    rise = rise_cm_series([100, 90, 80, 70, 60], px_per_cm=10, median_kernel=1)
    assert rise == [pytest.approx(v) for v in [0.0, 1.0, 2.0, 3.0, 4.0]]


def test_rise_cm_series_suppresses_single_frame_outlier():
    rows = [100, 100, 100, 999, 100, 100, 100, 100, 100]
    rise = rise_cm_series(rows, px_per_cm=10, median_kernel=5)
    # The outlier at index 3 sits in a window of otherwise-constant rows,
    # so the median filter should erase it (back to baseline, rise ~0).
    assert rise[3] == pytest.approx(0.0, abs=1e-6)
