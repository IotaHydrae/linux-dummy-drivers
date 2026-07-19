# V4L2 Dummy Video Driver

Its a dummy v4l2 video device driver that only implments the most basic functions. You can use this driver to learn how to write a video driver for a real physical hardware.

An animation will be displayed in a loop by default. [Learn how to replace](#how-to-replace-the-default-animation)

Build and install this driver
```
make test
```

Open a camera software like "cheese" and select this dummy-video device.


## How to Replace the default animation

### Extract frames from video file

```bash
sudo apt install ffmpeg imagemagick -y

ffmpeg -y -ss 23 -i jazz.mp4 \
-vframes 25 \
-fps_mode vfr \
-vf "scale=600:350:flags=lanczos:force_original_aspect_ratio=decrease,pad=600:350:(600-iw)/2:(350-ih)/2:black" \
-map_metadata -1 \
-c:v mjpeg \
-q:v 2 \
frame_%d.jpg

# convert to JPEG 1.1
mogrify -density 1x1 -strip frame_*.jpg
```

### Generate frames.h

Copy the generated "frames.h" to source dir

```bash
find -name "frame_*" | sed 's|./||' | sort -t'_' -k2n | xargs -I {} xxd -i {} > frames.h
```

Then rebuild this driver and reload:
```bash
make test
```

## Links

- drivers/meadia/usb/airspy
