# Dummy Framebuffer device

This driver is used to test kernel fbdev functionality. It registers a
480x320, 16bpp (RGB565) framebuffer backed by system memory (`vzalloc`) and
uses deferred IO (`fb_deferred_io`), so writes through the mmap'ed buffer are
collected and reported as dirty areas.

The device appears as `/dev/fbX` (check `/proc/fb` for the number) and in sysfs
as `/sys/class/dummy-fbclass/dummy-fbdev`.

## Build

```bash
make

# or index with clangd
bear -- make
```

`make` also builds the userspace test tools in `tests/`:

- `fb_rectangle` - draws rectangles into the framebuffer through mmap
- `fb_str` - renders 8x16 font text into the framebuffer through mmap

## Test

### Load and remove

Command below will do rmmod and insmod sequentially.

```bash
make test
```

If `rmmod` fails with `ERROR: Module dummy_fb is in use`, the framebuffer
console (fbcon) has taken over the device and holds a reference on the module.
Unbind fbcon first, then remove the module:

```bash
sudo sh -c 'echo 0 > /sys/class/vtconsole/vtcon1/bind'
sudo rmmod dummy_fb
```

The binding can be restored later with:

```bash
sudo sh -c 'echo 1 > /sys/class/vtconsole/vtcon1/bind'
```

Do **not** use `rmmod -f`: fbcon still points at the `fb_info` and a forced
unload can crash the kernel on the next access.

If `insmod` fails with `File exists`, the old module is still loaded - do the
unbind + `rmmod` steps above and retry.

### Userspace mmap tests

The test tools mmap the framebuffer device. The driver implements
`fbops->fb_mmap` with `fb_deferred_io_mmap()`, so touched pages are marked
dirty and handed to `dummy_fb_deferred_io()`:

```bash
sudo ./tests/fb_rectangle /dev/fb0
sudo ./tests/fb_str 10 10 0xFFFFFF "hello"
```

dmesg reports the dirty area on every deferred IO flush (values depend on what
was written):

```text
dummy-fbdev dummy-fbdev: dummy_fb_deferred_io, dirty area: (0, 0, 479, 319)
```

Note: `fb_rectangle` takes the device path as its first argument, while
`fb_str` currently hardcodes `/dev/fb1`.

### Screenshot using fbcat

If you are testing this driver on a PC, the dummy fb usually becomes
`/dev/fb1` (it is `/dev/fb0` on this WSL setup); check `/proc/fb` for the
actual number.

```bash
git clone https://github.com/jwilk/fbcat.git
cd fbcat
make
```

1. using fbcat take a ppm format screenshot

```bash
sudo ./fbcat /dev/fb1 > screenshot.ppm
```

2. using fbgrab take a png format screenshot

```bash
sudo ./fbgrab -d /dev/fb1 output.png
```
