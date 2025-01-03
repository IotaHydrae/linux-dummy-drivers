# Dummy Framebuffer device

This driver is used to test kernel fbdev functionality.

## Build

```bash
make

# or index with clangd
bear -- make
```

## Test

### Load and remove
Command below will do rmmod and insmod sequentially.

```bash
make test
```

### FB check using fbcat

If you are testing this driver though a PC, the fbdev normally will be `/dev/fb1`

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
