# RadarApi

Python api for interfacing with the Novelda X4. The X4 driver code is compiled in libRadarApi.so.
The python code uses ctypes to make calls to this shared library.

## Usage

To compile the libRadarApi.so, run the Create Shared Library task from tasks.json in the docker environment. 
The correct python version will be installed (RadarApi.py will be on the device which runs python 3.5.3)

There are two options (at the moment) for interfacing with the Python running on the device
1. Socket communication
2. Flask server -> requires flask be installed on the device

    #### Installing flask on the device
    The device runs python 3.5. For now, can replace the dist-packages folder on the device at /usr/local/lib/python3.5 with the one in the repository.

Place the libRadarApi.so on the device
Place RadarApi.py script on the device in the same directory

Run the RadarApi.py using command "python3 RadarApi.py" 
Use argument --flask to choose to run the flask server "python3 RadarApi.py --flask"

For client code, two example scripts are given in RadarClientExample.py (for socket) and RadarFlaskClientExample
If running flask, need to also run the run_flask_server.py to get the data from the device. The file name and save folder
can be adjusted using arguments to the script

Client-server IP addresses configuration:

1. Update ip addresses, upload folders, and credentials in RadarClientExample.py (or RadarFlaskClientExample.py)
server_ip = 'ip-address-of-holosis-imx8-board'
upload_folder = '/radar'
radar_client = RadarSocketClient(server_ip, 5044, "root", "root", "python3 RadarApi.py")

2. Update holosissystem/parameters/parameters.yaml, section 'Holosis connectivity' with servers credentials
Holosis Connectivity:
  ip: <host-pc-ip-address> #->(trego-compilation-server) #192.168.1.233(initial-ip-when-in-trego) #192.168.252.12(Leo), 192.168.1.232(in testing room), 192.168.1.233(Yael)
  app_path: NoveldaAppMatlab_01.05
  port: 5044
  user: <host-pc-username>
  password: <host-pc-password>
  python_command: python3 RadarApi.py


## Localhost example

After flashing the sdcard expand the rootfs partition to maximum sdcard capacity.
```bash
dd if=<sdcard img> of=<sdcard dev>
growpart <sdcard dev> 2
resize2fs <sdcard dev>
```

Build the RadarApi library. Copy both the RadarApi.py and libRadarApi.so
to the /usr/bin bath on the board.
```bash
cd .devcontainer
mkdir -p work
sudo ./run.sh -w $(pwd)/work -v $(pwd)/../:/workdir/RadarApi/
mkdir -p build && cd build
cmake ../ -DCMAKE_TOOLCHAIN_FILE=../cmake/aarch64-linux-gnu.cmake -DIMX8="True"
cmake --build .
scp RadarApi/libRadarApi.so ../RadarApi/RadarApi.py root@<ip>:/usr/bin
```

Copy the radar-client repository to the system, make sure branch localhost_ssh is checked out.
Run poetry install
```bash
cd radar-client
poetry lock
poetry install
```

To run the client first step is to start up the server.
```
RadarApi.py &
```
After that run the client script in another ssh terminal
```
poetry run python RadarClientExample.py
```


### Installing Holosis repo
Copy the repository to the system and edit the pyproject.toml in the follwing way:
```toml
radar-client = { path="<where client is copied>", develop=true }
#aws-manager = { git = "git@github.com:Holosis-Health/aws-manager.git", rev = "main" }
```
After the toml is edited you can install the depependecies with poetry.
```
poetry lock
poetry install
```
