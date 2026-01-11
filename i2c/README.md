dummy i2c adapter driver

Load i2c char dev driver first

```bash
sudo modprobe i2c-dev
```

build and install this driver

```bash
make
sudo insmod dummy_i2c.ko
```

test via `i2c-tools`

```bash
sudo apt install i2c-tools

❯ i2cdetect -l
i2c-0   unknown         dummy_i2c bus0                          N/A

❯ sudo i2cdetect -y 0
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:                         08 09 0a 0b 0c 0d 0e 0f
10: 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f
20: 20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f
30: 30 31 32 33 34 35 36 37 38 39 3a 3b 3c 3d 3e 3f
40: 40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f
50: 50 51 52 53 54 55 56 57 58 59 5a 5b 5c 5d 5e 5f
60: 60 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f
70: 70 71 72 73 74 75 76 77
```

instantiate I2C devices from user-space
```bash
echo eeprom | sudo tee /sys/bus/i2c/devices/i2c-0/new_device
```

## Log

adapter doesn't have a legal algo function, `drivers/i2c/i2c-core-base.c +1534`

```c
[27689.769983] i2c-core: adapter 'dummy_i2c bus0': no algo supplied!

/* Sanity checks */
if (WARN(!adap->name[0], "i2c adapter has no name"))
     goto out_list;

if (!adap->algo) {
     pr_err("adapter '%s': no algo supplied!\n", adap->name);
     goto out_list;
}
```
