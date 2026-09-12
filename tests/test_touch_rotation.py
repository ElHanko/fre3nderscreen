#!/usr/bin/env python3
"""Run with: python3 tests/test_touch_rotation.py"""

from pathlib import Path


ROT_NONE, ROT_90, ROT_180, ROT_270 = range(4)


def logical_resolution(rotation, physical_width, physical_height):
    if rotation in (ROT_90, ROT_270):
        return physical_height, physical_width
    return physical_width, physical_height


def lvgl_rotate(point, rotation, physical_width, physical_height):
    """Model LVGL 8's rotation after the input driver's read callback."""
    x, y = point
    if rotation in (ROT_180, ROT_270):
        x = physical_width - x - 1
        y = physical_height - y - 1
    if rotation in (ROT_90, ROT_270):
        x, y = physical_height - y - 1, x
    return x, y


def calibration_unrotate(point, rotation, logical_width, logical_height):
    """Expected inverse applied after affine calibration and before LVGL."""
    x, y = point
    if rotation == ROT_90:
        return y, logical_width - x - 1
    if rotation == ROT_180:
        return logical_width - x - 1, logical_height - y - 1
    if rotation == ROT_270:
        return logical_height - y - 1, x
    return point


physical_width, physical_height = 800, 480
for rotation in (ROT_NONE, ROT_90, ROT_180, ROT_270):
    logical_width, logical_height = logical_resolution(
        rotation, physical_width, physical_height
    )
    points = (
        (0, 0),
        (logical_width - 1, 0),
        (0, logical_height - 1),
        (logical_width - 1, logical_height - 1),
        (logical_width // 3, logical_height // 4),
    )
    for point in points:
        driver_point = calibration_unrotate(
            point, rotation, logical_width, logical_height
        )
        assert 0 <= driver_point[0] < physical_width
        assert 0 <= driver_point[1] < physical_height
        assert lvgl_rotate(
            driver_point, rotation, physical_width, physical_height
        ) == point

# 90 and 270 must be distinct inverses for an asymmetric point.
point = (37, 101)
assert calibration_unrotate(point, ROT_90, 480, 800) == (101, 442)
assert calibration_unrotate(point, ROT_270, 480, 800) == (698, 37)

# The old shared 270-degree inverse mirrors both logical axes at 90 degrees.
old_driver_point = calibration_unrotate(point, ROT_270, 480, 800)
assert lvgl_rotate(old_driver_point, ROT_90, 800, 480) == (442, 698)

# Keep the mathematical check tied to the production implementation.
source = " ".join(
    (Path(__file__).resolve().parents[1] / "lv_touch_calibration" / "lv_tc.c")
    .read_text()
    .split()
)
screen_source = " ".join(
    (Path(__file__).resolve().parents[1] / "lv_touch_calibration" / "lv_tc_screen.c")
    .read_text()
    .split()
)
expected_cases = (
    "case LV_DISP_ROT_NONE: break;",
    "case LV_DISP_ROT_90: transformedPoint.x = y; "
    "transformedPoint.y = lv_disp_get_hor_res(NULL) - x - 1; break;",
    "case LV_DISP_ROT_180: "
    "transformedPoint.x = lv_disp_get_hor_res(NULL) - x - 1; "
    "transformedPoint.y = lv_disp_get_ver_res(NULL) - y - 1; break;",
    "case LV_DISP_ROT_270: "
    "transformedPoint.x = lv_disp_get_ver_res(NULL) - y - 1; "
    "transformedPoint.y = x; break;",
)
for expected_case in expected_cases:
    assert expected_case in source
assert (
    "return calibResult.isValid ? lv_tc_unrotate_point(transformedPoint) "
    ": transformedPoint;"
) in source
assert (
    "lv_tc_screen_set_indicator_pos(screenObj, "
    "lv_tc_transform_point(tchPoint), true);"
) in screen_source

public_transform = source.split(
    "lv_point_t lv_tc_transform_point(lv_point_t point) {", 1
)[1].split("static lv_point_t lv_tc_transform_point_for_indev", 1)[0]
assert "lv_tc_unrotate_point" not in public_transform

print("touch rotation checks passed")
