```bash

sudo mount -t debugfs none /sys/kernel/debug 2>/dev/null

sudo sh -c "echo 'file drivers/base/firmware_loader/* +p' > /sys/kernel/debug/dynamic_debug/control"
```

```bash
[107588.879513] firmware_class: __allocate_fw_priv: fw-dummy-firmware.bin fw_priv=0000000090649678
[107588.879617] dummy-firmware-class dummy-firmware-dev: loading /lib/firmware/updates/6.6.114.1-microsoft-standard-WSL2+/dummy-firmware.bin failed for no such file or directory.
[107588.879627] dummy-firmware-class dummy-firmware-dev: loading /lib/firmware/updates/dummy-firmware.bin failed for no such file or directory.
[107588.879630] dummy-firmware-class dummy-firmware-dev: loading /lib/firmware/6.6.114.1-microsoft-standard-WSL2+/dummy-firmware.bin failed for no such file or directory.
[107588.879662] dummy-firmware-class dummy-firmware-dev: loading /lib/firmware/dummy-firmware.bin failed for no such file or directory.
[107588.879666] dummy-firmware-class dummy-firmware-dev: loading /lib/firmware/updates/6.6.114.1-microsoft-standard-WSL2+/dummy-firmware.bin.xz failed for no such file or directory.
[107588.879669] dummy-firmware-class dummy-firmware-dev: loading /lib/firmware/updates/dummy-firmware.bin.xz failed for no such file or directory.
[107588.879671] dummy-firmware-class dummy-firmware-dev: loading /lib/firmware/6.6.114.1-microsoft-standard-WSL2+/dummy-firmware.bin.xz failed for no such file or directory.
[107588.879673] dummy-firmware-class dummy-firmware-dev: loading /lib/firmware/dummy-firmware.bin.xz failed for no such file or directory.
[107588.879674] dummy-firmware-class dummy-firmware-dev: Direct firmware load for dummy-firmware.bin failed with error -2  
[107588.880324] firmware_class: __free_fw_priv: fw-dummy-firmware.bin fw_priv=0000000090649678 data=0000000000000000 size=0
[107588.880327] request_firmware failed
```