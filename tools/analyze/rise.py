"""Pure rise-detection logic: finding the dough/starter surface row in a
cropped grayscale photo, and turning a sequence of such rows (one per
photo, in time order) into a rise curve in cm. No file I/O here, so this
is covered by plain pytest without any dataset on disk - see
tests/test_rise.py.

Ported from the throwaway proef-1/proef-2 scripts at
tools/analyze/prototype/rise.py (row-mean brightness profile, smoothed
with a moving average, strongest row-to-row change as the surface edge),
generalized to a named, reusable region/sign instead of the two
hardcoded pixel boxes + per-edge sign choice in that prototype.
"""

from __future__ import annotations

import numpy as np
from scipy.signal import medfilt


def surface_row(gray: np.ndarray, smoothing: int = 1, sign: str = "abs") -> int:
    """Index of the dough/starter surface edge in a grayscale crop (rows x
    columns): the row whose mean brightness differs most from the row
    directly below it (e.g. the jar's empty headspace above vs. the
    dough below, or a dark dough edge against a lighter background).
    `gray` must have at least 2 rows.

    `smoothing` (odd-ish; even values are rounded down by one) averages
    the row-brightness profile over that many rows first (prototype's
    `np.convolve(..., np.ones(n)/n)`) to reduce single-row noise before
    looking for the edge; 1 disables smoothing. The smoothed ends are
    trimmed off before the edge search, since a moving average is
    unreliable right at the crop's top/bottom edge.

    `sign` picks which kind of transition counts as the surface:
    - "abs" (default): the largest brightness change in either
      direction - robust default, used for the "back" edge in F2e.
    - "rising": only a brightness *increase* going down the crop
      (dark-to-light) counts - the "front" edge in F2e's proef 1/2 setup
      saw this instead of a drop.
    - "falling": only a brightness *decrease* going down the crop
      (light-to-dark) counts.
    """
    if gray.ndim != 2 or gray.shape[0] < 2:
        raise ValueError("gray must be a 2D array with at least 2 rows")

    row_means = gray.mean(axis=1)

    half = max(0, smoothing // 2)
    if smoothing > 1:
        kernel = np.ones(smoothing) / smoothing
        profile = np.convolve(row_means, kernel, mode="same")
    else:
        profile = row_means

    diffs = np.diff(profile)
    if half > 0 and len(diffs) > 2 * half:
        diffs = diffs[half:-half]
        offset = half
    else:
        offset = 0

    if sign == "rising":
        idx = int(np.argmax(diffs))
    elif sign == "falling":
        idx = int(np.argmax(-diffs))
    elif sign == "abs":
        idx = int(np.argmax(np.abs(diffs)))
    else:
        raise ValueError(f"Unknown sign {sign!r}, expected 'abs', 'rising' or 'falling'")
    return offset + idx


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
