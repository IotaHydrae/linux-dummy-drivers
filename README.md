# Linux dummy drivers

This repository contains a collection of **Linux dummy drivers**. These drivers are designed with reduced complexity to simulate various hardware components and are intended for learning purposes.

With this repository, you can easily understand how the driver works when invoked from
userspace tools. It is a good starting point if you want to develop an actual hardware driver.

## System Info

| -              | -                              |
| -------------- | ------------------------------ |
| Machine        | Raspberry Pi 5 Model B Rev 1.0 |
| Distro         | Debian GNU/Linux 13 (trixie)   |
| Kernel Version | 6.12.62+rpt-rpi-2712           |

## Development status

- [x] v4l2 dummy camera video dev
- [x] legacy framebuffer dev
- [x] drm_simple_pipe
- [x] i2c adapter
- [x] spi master
- [x] gpiochip
- [ ] usb host
- [ ] sound
- [ ] dma engine

## How to use

```bash
git clone https://github.com/IotaHydrae/linux-dummy-drivers.git
```

See `README.md` file in each driver directory.

## Reference

### DMA Engine

### i2c adapter

- [Kernel I2C/SMBus Subsystem Documentation](https://www.kernel.org/doc/html/latest/i2c/index.html)
- [i2c-suniv (Yet another i2c adapter driver for Allwinner suniv series soc)](https://github.com/IotaHydrae/i2c-suniv)
