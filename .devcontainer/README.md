# README

---

## Steps to build Holosis SDK and SW:

### 1. Download the SDK to a Docker Volume Folder

Run the following command to download the SDK sources to the specified folder( the script is located in imx8-sdk dir):

```bash
download-sdk-sources.sh <sdk_volume_folder>
```



---

### 2. Run/Build the Docker Devcontainer

#### To get help for the script:
```bash
./run.sh --help
```

#### To run the container with `workdir` mapped to `holosis-work` and `<sdk_volume_folder>` used in step #1 mounted inside the Docker container:
```bash
./run.sh -w holosis-work -v ./holosis-work/RadarApi/imx8-sdk/<sdk_volume_folder>:/workdir/RadarApi/imx8-sdk/<sdk_volume_folder>
```

#### To rebuild the Docker image:
```bash
./run.sh -w holosis-work -v  ./holosis-work/RadarApi/imx8-sdk/<sdk_volume_folder>:/workdir/RadarApi/imx8-sdk/<sdk_volume_folder> -b
```

---

### 3. Build the SDK in the Docker Container

#### Switch to the SDK installed directory:
```bash
cd /workdir/RadarApi/imx8-sdk/<sdk_volume_folder>/imx8mp_build
```

#### Build the SDK:
```bash
DISTRO=debian ./build-sdk.sh --all
```

#### Build SDK Components:
```bash
./build-sdk.sh --help
```

#### Running qemu with the kernel and rootfs:
```bash
sudo qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a57 \
    -m 2048 \
    -nographic \
    -kernel images/tmp/linux/boot/Image \
    -append "root=/dev/vda console=ttyAMA0 ip=dhcp rw " \
    -drive file=images/tmp/rootfs.ext4,format=raw,if=virtio \
    -netdev user,id=net0,hostfwd=tcp::2222-:22 \
    -device virtio-net-device,netdev=net0
```