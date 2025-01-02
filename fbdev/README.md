# Dummy Framebuffer device

This driver is used to test kernel fbdev functionality.

## Build

```bash
make

# or index with clangd
bear -- make
```

## Test

Command below will do rmmod and insmod sequentially.

```bash
make test
```