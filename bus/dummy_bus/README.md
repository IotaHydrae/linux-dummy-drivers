# Dummy Bus example

All buses define their own set of devices and drivers.

```
Platform
--------
platform_device
platform_driver

SPI
---
spi_device
spi_driver

I2C
---
i2c_client
i2c_driver

PCI
---
pci_dev
pci_driver

USB
---
usb_device
usb_driver
```

## Why not use struct device?

For example, SPI. If there are only:
```c
struct device
```

Other SPI device configs, such as:
```
Chip select number
SPI mode
SPI frequency
bits_per_word
```
Have nowhere to store. can't put them in struct device.

## Composition

For example, Platform:
```c
struct platform_device {
	...
	struct device dev;
};
```
SPI:
```c
struct spi_device {
	...
	struct device dev;
};
```

All buses treat struct device as a 'base class'.
This is a classic way of simulating object-oriented inheritance in the C language.
