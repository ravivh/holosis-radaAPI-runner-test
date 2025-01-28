# README

---

## Steps to build Holosis SDK and SW:

### 1. Download the SDK to a Docker Volume Folder

Run the following command to download the SDK sources to the specified folder:

```bash
download-sdk-sources.sh <sdk_volume_folder>
```

#### Example:
If `holosis-work` is the root folder of all development and is mapped in Docker to `/workdir`, the following command will download the SDK sources to `holosis-work/holosis-bsp` and create `holosis-bsp` if it does not exist:

```bash
./download-sdk-sources.sh holosis-work/holosis-bsp
```

---

### 2. Run/Build the Docker Devcontainer

#### To get help for the script:
```bash
./run.sh --help
```

#### To run the container with `workdir` mapped to `holosis-work` and `holosis-bsp` mounted inside the Docker container:
```bash
./run.sh -w holosis-work -v ./holosis-work/holosis-bsp/:/workdir/holosis-bsp
```

#### To rebuild the Docker image:
```bash
./run.sh -w holosis-work -v ./holosis-work/holosis-bsp/:/workdir/holosis-bsp -b
```

---

### 3. Build the SDK in the Docker Container

#### Switch to the SDK installed directory:
```bash
cd /workdir/holosis-bsp/imx8mp_build
```

#### Build the SDK:
```bash
DISTRO=debian ./runme.sh
