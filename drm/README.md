# Dummy DRM driver

This driver use drm_simple_pipe to implement a simple DRM driver. Once this driver is loaded, a 480x320 fb will be created.

## Build & Install

```bash
make
sudo insmod dummy-drm.ko
```

Log of driver initialization:

```bash
               dummy_drm_init
[  581.309713] dummy-drm: dummy_drm_dev_init
[  581.309714] dummy-drm: dummy_drm_dev_init_with_formats
[  581.309721] dummy-drm: mode: 480x320
[  581.309831] [drm] Initialized dummy-drm 1.0.0 20250111 for dummy-drmdev on minor 0
[  581.309839] dummy-drm: dummy_drm_pipe_mode_valid
[  581.310237] dummy-drm: dummy_drm_pipe_mode_valid
[  581.310242] dummy-drm: dummy_drm_pipe_update
[  581.310242] dummy-drm: x1: 0, y1: 0, x2: 480, y2: 320
[  581.310243] dummy-drm: dummy_drm_pipe_enable
[  581.310257] Console: switching to colour frame buffer device 60x40
[  581.310265] dummy-drm: dummy_drm_pipe_update
[  581.310265] dummy-drm: x1: 0, y1: 0, x2: 480, y2: 320
[  581.310312] dummy-drmclass dummy-drmdev: [drm] fb0: dummy-drmdrmfb frame buffer device
```

You will see driver printing dirty area info to dmesg from drm_pipe_update function.

```bash
[ 1165.018451] dummy-drm: dummy_drm_pipe_update
[ 1165.018454] dummy-drm: x1: 184, y1: 48, x2: 192, y2: 64
```

## Test

Use command below to dump the framebuffer, assume /dev/fb0 is used.
```bash
make dump
```

You will see a dump file named "dump.png" in current directory.

On my WSL machine, it looks like this:

![dump](./dump.png)
