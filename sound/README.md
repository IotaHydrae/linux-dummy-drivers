# Dummy Sound Card Driver

## Description

This driver is a dummy ALSA sound card driver for Linux. It is designed to simulate a sound card with basic functionalities.

## How to test

### Checkout ALSA Device

Though procfs

```bash
cat /proc/asound/cards

# you will get some output like this
 0 [vc4hdmi0       ]: vc4-hdmi - vc4-hdmi-0
                      vc4-hdmi-0
 1 [vc4hdmi1       ]: vc4-hdmi - vc4-hdmi-1
                      vc4-hdmi-1
 2 [Sound          ]: dummy_sound - Dummy Sound
                      Dummy Sound Card
```

Checkout PCM device /dev/snd/xxx

```bash
ls -lh /dev/snd

total 0
drwxr-xr-x 2 root root       80 Mar 17 08:04 by-path
crw-rw---- 1 root audio 116,  3 Mar 17 08:04 controlC0
crw-rw---- 1 root audio 116,  5 Mar 17 08:04 controlC1
crw-rw---- 1 root audio 116,  8 Mar 17 12:10 controlC2
crw-rw---- 1 root audio 116,  2 Mar 17 08:04 pcmC0D0p
crw-rw---- 1 root audio 116,  4 Mar 17 08:04 pcmC1D0p
crw-rw---- 1 root audio 116,  7 Mar 17 12:10 pcmC2D0c
crw-rw---- 1 root audio 116,  6 Mar 17 12:10 pcmC2D0p
crw-rw---- 1 root audio 116,  1 Mar 17 08:04 seq
crw-rw---- 1 root audio 116, 33 Mar 17 08:04 timer
```

/dev/snd/controlC2 is the control device for the dummy sound card. /dev/snd/pcmC2D0p is the playback device. /dev/snd/pcmC2D0c is the capture device.

