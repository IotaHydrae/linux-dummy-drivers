#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
"""fbview.py - live preview of a Linux framebuffer (/dev/fbX).

Modes:
  gui  - live window (default when a display is available; needs python3-tk)
  ansi - terminal preview using truecolor half blocks (works over SSH/WSL)
  once - print a single frame (PPM to stdout, or ANSI with --ansi)

Examples:
  sudo python3 tools/fbview.py                 # auto-detect /dev/fbN, GUI
  sudo python3 tools/fbview.py /dev/fb0 --fps 10
  sudo python3 tools/fbview.py --ansi
  sudo python3 tools/fbview.py --once > frame.ppm
"""

import argparse
import base64
import fcntl
import os
import shutil
import struct
import sys
import time

try:
    import tkinter as tk
except ImportError:
    tk = None

FBIOGET_VSCREENINFO = 0x4600
VAR_FMT = "<" + "I" * 40
VAR_SIZE = struct.calcsize(VAR_FMT)

_VAR_NAMES = (
    "xres", "yres", "xres_virtual", "yres_virtual", "xoffset", "yoffset",
    "bits_per_pixel", "grayscale",
    "red_offset", "red_length", "red_msb",
    "green_offset", "green_length", "green_msb",
    "blue_offset", "blue_length", "blue_msb",
    "transp_offset", "transp_length", "transp_msb",
    "nonstd", "activate", "height", "width", "accel_flags", "pixclock",
    "left_margin", "right_margin", "upper_margin", "lower_margin",
    "hsync_len", "vsync_len", "sync", "vmode", "rotate", "colorspace",
    "reserved0", "reserved1", "reserved2", "reserved3",
)


def read_var(fd):
    buf = fcntl.ioctl(fd, FBIOGET_VSCREENINFO, b"\0" * VAR_SIZE)
    return dict(zip(_VAR_NAMES, struct.unpack(VAR_FMT, buf)))


def frame_size(var):
    bpp = (var["bits_per_pixel"] + 7) // 8
    stride = var["xres_virtual"] * bpp
    return stride * var["yres"]


class Framebuffer:
    def __init__(self, path):
        self.path = path
        self.fd = os.open(path, os.O_RDONLY)
        self.var = read_var(self.fd)
        self.size = frame_size(self.var)

    def read_frame(self):
        return os.pread(self.fd, self.size, 0)

    def close(self):
        os.close(self.fd)


def open_fb(path):
    try:
        return Framebuffer(path)
    except OSError as e:
        raise SystemExit("%s: %s (try sudo)" % (path, e.strerror or e))


def find_fb():
    for i in range(32):
        path = "/dev/fb%d" % i
        try:
            return Framebuffer(path)
        except OSError:
            continue
    raise SystemExit("no /dev/fbN found (try sudo)")


class Converter:
    """Convert raw framebuffer bytes to packed 8-bit RGB (w*h*3)."""

    def __init__(self, var):
        self.w = var["xres"]
        self.h = var["yres"]
        self.bpp = var["bits_per_pixel"]
        self.channels = [
            (var[k + "_offset"], var[k + "_length"])
            for k in ("red", "green", "blue")
        ]
        self.masks = [((1 << ln) - 1) << off for off, ln in self.channels]
        # Fast path: standard RGB565 (offsets 11/5/0, lengths 5/6/5)
        self._table = None
        if self.bpp == 16 and self.channels == [(11, 5), (5, 6), (0, 5)]:
            self._table = [None] * 65536
            for p in range(65536):
                r = (p >> 11) & 0x1F
                g = (p >> 5) & 0x3F
                b = p & 0x1F
                self._table[p] = bytes((
                    r << 3 | r >> 2,
                    g << 2 | g >> 4,
                    b << 3 | b >> 2,
                ))

    def convert(self, data):
        n = self.w * self.h
        if self._table is not None:
            px = struct.unpack("<%dH" % n, data[: n * 2])
            return b"".join(self._table[p] for p in px)
        if self.bpp == 24 and self.channels == [(16, 8), (8, 8), (0, 8)]:
            mv = memoryview(data)[: n * 3]
            out = bytearray(n * 3)
            out[0::3] = mv[2::3]  # R
            out[1::3] = mv[1::3]  # G
            out[2::3] = mv[0::3]  # B
            return bytes(out)
        if self.bpp == 32:
            return self._convert32(data, n)
        raise RuntimeError("unsupported bpp %d" % self.bpp)

    def _convert32(self, data, n):
        (ro, rl), (go, gl), (bo, bl) = self.channels
        rm, gm, bm = self.masks
        out = bytearray(n * 3)
        k = 0
        for p in struct.unpack("<%dI" % n, data[: n * 4]):
            out[k] = ((p & rm) >> ro) << (8 - rl)
            out[k + 1] = ((p & gm) >> go) << (8 - gl)
            out[k + 2] = ((p & bm) >> bo) << (8 - bl)
            k += 3
        return bytes(out)


def ansi_render(rgb, w, h, cols, rows):
    """Downscale RGB to cols x rows truecolor half-block cells."""
    cw = max(1, w // cols)
    ch = max(1, h // (rows * 2))
    lines = []
    for tr in range(rows):
        cells = []
        for tc in range(cols):
            x = min(w - 1, tc * cw + cw // 2)
            y0 = min(h - 1, tr * 2 * ch + ch // 2)
            y1 = min(h - 1, (tr * 2 + 1) * ch + ch // 2)
            i0 = (y0 * w + x) * 3
            i1 = (y1 * w + x) * 3
            cells.append(
                "\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm\u2580"
                % (rgb[i0], rgb[i0 + 1], rgb[i0 + 2],
                   rgb[i1], rgb[i1 + 1], rgb[i1 + 2])
            )
        lines.append("".join(cells))
    return "\n".join(lines) + "\x1b[0m"


def run_ansi(fb, fps, maxw, maxh, once=False, fill=False):
    conv = Converter(fb.var)
    sys.stdout.write("\x1b[?25l\x1b[2J")
    try:
        while True:
            size = shutil.get_terminal_size((120, 40))
            max_cols = maxw or max(1, size.columns - 2)
            max_rows = maxh or max(1, (size.lines - 2) // 2)
            if fill:
                cols, rows = max_cols, max_rows
                cell = 0
            else:
                # Uniform downscale so the preview keeps the fb's aspect ratio.
                cell = max(
                    1,
                    (conv.w + max_cols - 1) // max_cols,
                    (conv.h + max_rows * 2 - 1) // (max_rows * 2),
                )
                cols = max(1, conv.w // cell)
                rows = max(1, conv.h // (cell * 2))
            rgb = conv.convert(fb.read_frame())
            detail = "" if cell == 0 else (", %dpx/cell" % cell)
            header = ("%s: %dx%d %dbpp (scaled to %dx%d cells%s)"
                      % (fb.path, conv.w, conv.h, fb.var["bits_per_pixel"],
                         cols, rows, detail))
            frame = "\x1b[7m" + header + "\x1b[0m\n" + \
                    ansi_render(rgb, conv.w, conv.h, cols, rows)
            sys.stdout.write("\x1b[H" + frame)
            sys.stdout.flush()
            if once:
                return
            time.sleep(1.0 / fps)
    except KeyboardInterrupt:
        pass
    finally:
        sys.stdout.write("\x1b[0m\x1b[?25h\n")
        sys.stdout.flush()


def run_once_ppm(fb):
    conv = Converter(fb.var)
    rgb = conv.convert(fb.read_frame())
    sys.stdout.buffer.write(
        b"P6\n%d %d\n255\n" % (conv.w, conv.h) + rgb
    )


def run_gui(fb, fps):
    if tk is None:
        print("GUI mode needs python3-tk: sudo apt install python3-tk",
              file=sys.stderr)
        return False
    conv = Converter(fb.var)
    w, h = conv.w, conv.h

    def make_image():
        rgb = conv.convert(fb.read_frame())
        ppm = b"P6\n%d %d\n255\n" % (w, h) + rgb
        return tk.PhotoImage(data=base64.b64encode(ppm).decode("ascii"))

    try:
        first = make_image()
    except Exception as e:
        print("GUI preview failed (%s), falling back to ANSI" % e, file=sys.stderr)
        return False

    root = tk.Tk()
    root.title("fbview: %s (%dx%d %dbpp)" % (
        fb.path, w, h, fb.var["bits_per_pixel"]))
    label = tk.Label(root)
    label.pack()
    label.configure(image=first)
    label.image = first

    def tick():
        try:
            img = make_image()
            label.configure(image=img)
            label.image = img
        except Exception as e:
            print("preview error: %s" % e, file=sys.stderr)
        root.after(max(1, int(1000 / fps)), tick)

    root.after(max(1, int(1000 / fps)), tick)
    root.mainloop()
    return True


def main():
    ap = argparse.ArgumentParser(
        description="Real-time preview of a Linux framebuffer (/dev/fbX)")
    ap.add_argument("device", nargs="?", default=None,
                    help="/dev/fbN (default: auto-detect)")
    ap.add_argument("--fps", type=float, default=10.0,
                    help="refresh rate in frames per second (default: 10)")
    ap.add_argument("--mode", choices=("auto", "gui", "ansi"), default="auto",
                    help="preview mode (default: auto)")
    ap.add_argument("--once", action="store_true",
                    help="print a single frame and exit (PPM to stdout)")
    ap.add_argument("--maxw", type=int, default=0,
                    help="max columns for ANSI mode")
    ap.add_argument("--maxh", type=int, default=0,
                    help="max rows for ANSI mode")
    ap.add_argument("--fill", action="store_true",
                    help="stretch the preview to fill the terminal "
                         "(default: keep the fb's aspect ratio)")
    args = ap.parse_args()

    fb = open_fb(args.device) if args.device else find_fb()
    var = fb.var
    print("%s: %dx%d %dbpp (virtual %dx%d)" % (
        fb.path, var["xres"], var["yres"], var["bits_per_pixel"],
        var["xres_virtual"], var["yres_virtual"]), file=sys.stderr)

    if args.once:
        if args.mode == "ansi":
            run_ansi(fb, args.fps, args.maxw, args.maxh,
                     once=True, fill=args.fill)
        else:
            run_once_ppm(fb)
        return

    mode = args.mode
    if mode == "auto":
        has_display = bool(os.environ.get("DISPLAY") or
                           os.environ.get("WAYLAND_DISPLAY"))
        if not has_display:
            print("note: DISPLAY/WAYLAND_DISPLAY not set, using ANSI mode "
                  "(running under sudo? use `make preview`, or add your user "
                  "to the video group and run without sudo)",
                  file=sys.stderr)
            mode = "ansi"
        elif tk is None:
            print("note: python3-tk not available, using ANSI mode "
                  "(sudo apt install python3-tk)", file=sys.stderr)
            mode = "ansi"
        else:
            mode = "gui"

    if mode == "gui" and not run_gui(fb, args.fps):
        run_ansi(fb, args.fps, args.maxw, args.maxh, fill=args.fill)
    elif mode == "ansi":
        run_ansi(fb, args.fps, args.maxw, args.maxh, fill=args.fill)


if __name__ == "__main__":
    main()
