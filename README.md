# Linux dummy drivers

This repository contains a collection of **Linux dummy drivers**. These drivers are designed with reduced complexity to simulate various hardware components and are intended for learning purposes.

With this repository, you can easily understand how the driver works when invoked from
userspace tools. It is a good starting point if you want to develop an actual hardware driver.

## System Info

| -              | -                                  |
| -------------- | ---------------------------------- |
| Distro         | Ubuntu 26.04 LTS                   |
| Kernel Version | 6.18.40.1-microsoft-standard-WSL2+ |

## Development status

- [x] [hello world](hello/)
- [x] [dummy bus](bus/dummy_bus/README.md) (basic_bus / dummy_bus)
- [x] [v4l2 dummy camera video dev](v4l2/README.md)
- [x] [legacy framebuffer dev](fbdev/README.md)
- [ ] drm
  - [x] [drm_simple_display_pipe](drm/drm_simple_display_pipe/README.md)
  - [ ] full virtual drm driver (complete DRM chain, without drm_simple_display_pipe)
- [x] [i2c adapter](i2c/README.md)
- [x] [spi master](spi/README.md)
- [x] [gpiochip](gpio/README.md)
- [x] [firmware loader](firmware/README.md)
- [x] [regulator](regulator/README.md)
- [x] [static_call](static_call/README.md)
- [ ] usb host
- [ ] sound - ALSA
- [ ] dma engine

## How to use

```bash
git clone https://github.com/IotaHydrae/linux-dummy-drivers.git
```

See `README.md` file in each driver directory.

## Reference

### ALSA

- [Writing an ALSA Driver](https://www.kernel.org/doc/html/latest/sound/kernel-api/writing-an-alsa-driver.html)

### DMA Engine

- [suniv-dma](https://github.com/IotaHydrae/suniv-dma) - The DMA Engine driver for Allwinner Suniv SoC

### i2c adapter

- [Kernel I2C/SMBus Subsystem Documentation](https://www.kernel.org/doc/html/latest/i2c/index.html)
- [i2c-suniv (Yet another i2c adapter driver for Allwinner suniv series soc)](https://github.com/IotaHydrae/i2c-suniv)
