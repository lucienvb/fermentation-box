"""Pure rise-detection logic: finding the dough/starter surface row in a
cropped grayscale photo, and turning a sequence of such rows (one per
photo, in time order) into a rise curve in cm. No file I/O here, so this
is covered by plain pytest without any dataset on disk - see
tests/test_rise.py.
"""

from __future__ import annotations

import numpy as np
from scipy.signal import medfilt


def surface_row(gray: np.ndarray) -> int:
    """Index of the strongest horizontal edge in a grayscale crop (rows x
    columns), used as a proxy for the dough/starter surface: the row
    whose mean brightness differs most from the row directly below it
    (e.g. the jar's empty headspace above vs. the dough below). `gray`
    must have at least 2 rows.
    """
    if gray.ndim != 2 or gray.shape[0] < 2:
        raise ValueError("gray must be a 2D array with at least 2 rows")
    row_means = gray.mean(axis=1)
    diffs = np.abs(np.diff(row_means))
    return int(np.argmax(diffs))


def rise_cm_series(surface_rows: list[int], px_per_cm: float, median_kernel: int = 5) -> list[float]:
    """Converts a sequence of surface_row() results (same crop region,
    one per photo, in time order) into a rise curve in cm relative to the
    first frame, with a median filter against single-frame outliers (a
    reflection or shadow briefly confusing surface_row()). Positive
    values mean the surface moved up (rose) since the first frame; the
    row axis increases downward, so a rise is a *decrease* in row index.
    """
    if not surface_rows:
        return []
    if px_per_cm <= 0:
        raise ValueError("px_per_cm must be > 0")

    rows = np.asarray(surface_rows, dtype=float)

    kernel = min(median_kernel, len(rows))
    if kernel % 2 == 0:
        kernel -= 1
    kernel = max(1, kernel)
    smoothed = medfilt(rows, kernel_size=kernel) if kernel > 1 else rows

    baseline = smoothed[0]
    rise = (baseline - smoothed) / px_per_cm
    return rise.tolist()
