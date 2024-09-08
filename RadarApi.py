import argparse
import ctypes
try:
    from flask import Flask, request, jsonify
    flask_flag = 1
except ImportError:
    print("Flask not installed. Resorting to socket")
    flask_flag = 0
import json
import math
import queue
import select
import socket
import struct
import sys
import threading
import time
from typing import Callable, Dict, Tuple
import urllib
import uuid





# Right now communication is implemented using sockets; I think we should also examine the possiblity of using lightweight
# servers as Yaniv mentioned. This could also make it possible to upload data to our server without being on the
# same network. I guess once everything is on device we would want either the flask server or other IPC in any case.


X4DRIVER_METERS_PER_BIN = 1*1.5e8/23.328e9

def x4_calculate_bin_number(start: float, end: float, downconversion_enabled: int) -> int:
    """
    Does the same calculation as in the radar driver to get the correct number of bins
    Inputs:
        start(float) start of the frame area
        end(float) end of the frame area
        downconversion_enabled(int) 1 if downconversion is enabled, 0 otherwise
    Outputs:
        frame_read_size(int) number of bins
    """
    frame_area_start_requested = start
    frame_area_end_requested = end
    # Down-conversion requires 4 first bins to initialize filter
    # Down-conversion decimates the number of bins by 8
    NumberOfBinsDiscarded = 0
    DecimationFactor = 0
    if downconversion_enabled == 1:
        NumberOfBinsDiscarded=4
        DecimationFactor=8
        bytes_per_counter = 6
    else:
        NumberOfBinsDiscarded=0
        DecimationFactor=1
        bytes_per_counter=3

    NumberOfBinsPerFrame=1536/DecimationFactor

    FrameAreaStart = start
    FrameAreaStop = end
    FrameAreaOffset = 0
    
    dR_bin = X4DRIVER_METERS_PER_BIN * DecimationFactor
    R_frame_step = X4DRIVER_METERS_PER_BIN * 96
    R_FrameStart_discarded = NumberOfBinsDiscarded * dR_bin

    # Find the required RXWait-TXWait diff to capture the requested start
    diff_RxTxWait = (FrameAreaStart - FrameAreaOffset) // R_frame_step 
    if diff_RxTxWait < -1:
        diff_RxTxWait = -1

    # Set RXWait according to TXWait and required diff
    TxWait = 1
    RxWait = TxWait+diff_RxTxWait
    
    # Calculate limits of captured frame
    FrameStart_quantized = diff_RxTxWait * R_frame_step + FrameAreaOffset

    FrameAreaStart = max(FrameAreaStart, FrameStart_quantized)
    FrameAreaStop = min(FrameAreaStop, dR_bin * (NumberOfBinsPerFrame - NumberOfBinsDiscarded - 1) + FrameStart_quantized)
    FrameAreaStop = max(FrameAreaStop, FrameAreaStart)

    # Find start bin index in the captured frame
    diff_eps_meter=1e-4
    bin_count_diff = (FrameAreaStart - FrameStart_quantized) / dR_bin
    diff = bin_count_diff - math.ceil(bin_count_diff)
    FrameAreaStart_quantized_ind = 0
    if abs(diff) < diff_eps_meter:
        FrameAreaStart_quantized_ind = math.ceil(bin_count_diff) + NumberOfBinsDiscarded
    else:
        FrameAreaStart_quantized_ind = math.floor(bin_count_diff) + NumberOfBinsDiscarded

    # Find stop bin index in the captured frame
    bin_count_diff = (FrameAreaStop - FrameStart_quantized) / dR_bin
    diff = bin_count_diff - math.floor(bin_count_diff)
    FrameAreaStop_quantized_ind = 0
    if abs(diff) < diff_eps_meter:
        FrameAreaStop_quantized_ind = math.floor(bin_count_diff) + NumberOfBinsDiscarded
    else:
        FrameAreaStop_quantized_ind = math.ceil(bin_count_diff) + NumberOfBinsDiscarded

    FrameAreaStart_quantized = (FrameAreaStart_quantized_ind - NumberOfBinsDiscarded) * dR_bin + FrameStart_quantized
    FrameAreaStop_quantized = (FrameAreaStop_quantized_ind - NumberOfBinsDiscarded) * dR_bin + FrameStart_quantized

    if downconversion_enabled==1:
        frame_area_start_ram_line = FrameAreaStart_quantized_ind
        frame_area_end_ram_line = FrameAreaStop_quantized_ind

        frame_area_start = FrameAreaStart_quantized
        frame_area_end  = FrameAreaStop_quantized
    else:
        frame_area_start_ram_line = FrameAreaStart_quantized_ind / 4
        frame_area_end_ram_line = FrameAreaStop_quantized_ind / 4

        frame_area_start_bin_offset = FrameAreaStart_quantized_ind % 4
        if (FrameAreaStop_quantized_ind % 4) == 0:
            frame_area_end_bin_offset = 0
            frame_area_end_ram_line -= 1
        else:
            frame_area_end_bin_offset = 4 - (FrameAreaStop_quantized_ind % 4)

        frame_area_start = FrameAreaStart_quantized
        frame_area_end  = FrameAreaStop_quantized

    if downconversion_enabled:
        bins = int((frame_area_end_ram_line - frame_area_start_ram_line) * 2)
        bytes_to_read = (frame_area_end_ram_line - frame_area_start_ram_line + 1) * 2 * bytes_per_counter
        frame_read_size = bytes_to_read
    else:
        bytes_to_read = (frame_area_end_ram_line - frame_area_start_ram_line + 1) * 4 * bytes_per_counter
        frame_read_size = bytes_to_read
        bins = int((frame_area_end_ram_line - frame_area_start_ram_line) * 4)
    return bins


# TODO add reset radar
class PythonRadarApi:
    frequency_dict = {'US': 4, 'EU': 3}
    transmit_power_dict = {'OFF': 0, 'LOW': 1, 'MEDIUM': 2,
                           'HIGH': 3}

    
    init_param_names = ['frequency', 'transmit_gain', 'dac_min', 'dac_max', 'tzero_ns', 'iterations']
    dynamic_param_names = ['antenna_numbers', 'repetitions', 'r_start', 'start_to_stop', 'prf_div', 'pps', 'fps', 'downconversion_enabled']

    def __init__(self, timeout: int=100, so_path: str="./libRadarApi.so", debug: int=0,
                 queue_size: int=0):
        """
        Class for the radar api in python. Uses ctypes to call the c functions
        which set the radar parameters and request data from the radar.
        Each request is sent as json in the same format and added to a request queue.
        The requests are processed sequentially.
        Inputs:
            timeout(int) - timeout for the radar to respond. 
            so_path(str) - path to the shared object file
            debug(int) - debug flag
            queue_size(int) - size of the request queue. The default, 0, indicates an 
                                infinite queue size.
        """
        self.timeout = timeout
        # types
        self.mylib = ctypes.CDLL(so_path)
        self.mylib.radar_init.argtypes= [ctypes.c_int, ctypes.c_int, ctypes.c_int,
                            ctypes.c_int, ctypes.c_float, ctypes.c_int,ctypes.c_int]
        self.mylib.radar_init.restype=ctypes.c_int


        self.mylib.radar_request.argtypes=[ctypes.POINTER(ctypes.c_int), ctypes.c_int,
                              ctypes.c_int, ctypes.POINTER(ctypes.c_float),
                              ctypes.c_float, ctypes.c_float, ctypes.c_uint8,
                              ctypes.c_uint16, ctypes.c_float, ctypes.c_uint8]

        self.mylib.radar_request.restype=ctypes.c_int

        self.mylib.radar_change_parameters.argtypes=[ctypes.c_int, ctypes.c_int, ctypes.c_int,
                                                    ctypes.c_int, ctypes.c_float, ctypes.c_int]
        self.mylib.radar_change_parameters.restype=ctypes.c_int
        
        self.debug = 0
        self.request_queue = queue.Queue(maxsize=queue_size)
        # constant params
        self.frequency = None
        self.transmit_gain = None
        self.dac_min = None
        self.dac_max = None
        self.tzero_ns = None
        self.iterations = None

    def initial_setup(self, frequency: str, transmit_gain: str, dac_min: int, dac_max: int,
                      tzero_ns: float, iterations:int) -> int:
        """
        Initialize the radar with the given parameters. these are the parameters
        which are not expected to change during the radar operation. Typically
        they can be initialized once at startup and left as is.
        Inputs:
            frequency(str) - frequency of the radar. 'US' or 'EU'
            transmit_gain(str) - transmit power. 'OFF', 'LOW', 'MEDIUM', 'HIGH'
            dac_min(int) - minimum dac value
            dac_max(int) - maximum dac value
            tzero_ns(float) - tzero in ns - hardware offset for the radar sampling
            iterations(int) - number of iterations for radr integration
        Outputs:
            result(int) - result of the radar initialization
        """
        self.frequency = self.frequency_dict[frequency]
        self.transmit_gain = self.transmit_power_dict[transmit_gain]
        self.dac_min = dac_min
        self.dac_max = dac_max
        self.tzero_ns = tzero_ns
        self.r_zero = self.tzero_ns*3e8/(1e9*2)
        self.iterations = iterations
        result = self.mylib.radar_init(self.frequency, self.transmit_gain,
                              self.dac_min, self.dac_max, self.tzero_ns,
                              self.iterations, self.debug)
        return result

    def change_params(self, frequency: str, transmit_gain: str, dac_min: int, dac_max: int,
                      tzero_ns: float, iterations:int) -> int:
        """
        Initialize the radar with the given parameters. these are the parameters
        which are not expected to change during the radar operation. Typically
        they can be initialized once at startup and left as is.
        Inputs:
            frequency(str) - frequency of the radar. 'US' or 'EU'
            transmit_gain(str) - transmit power. 'OFF', 'LOW', 'MEDIUM', 'HIGH'
            dac_min(int) - minimum dac value
            dac_max(int) - maximum dac value
            tzero_ns(float) - tzero in ns - hardware offset for the radar sampling
            iterations(int) - number of iterations for radr integration
        Outputs:
            result(int) - result of the radar initialization
        """
        self.frequency = self.frequency_dict[frequency]
        self.transmit_gain = self.transmit_power_dict[transmit_gain]
        self.dac_min = dac_min
        self.dac_max = dac_max
        self.tzero_ns = tzero_ns
        self.r_zero = self.tzero_ns*3e8/(1e9*2)
        self.iterations = iterations
        result = self.mylib.radar_change_parameters(self.frequency, self.transmit_gain,
                                       self.dac_min, self.dac_max, self.tzero_ns,
                                       self.iterations)
        return result


    def radar_request(self, antenna_numbers: list, repetitions: int, r_start: float, start_to_stop: float,
                      prf_div: int, pps: int, fps: float, downconversion_enabled: int) -> (int, ctypes.c_float):
        """
        Request data from the radar with the given parameters. 
        The data acquisition is defined by the parameters and the antenna numbers.
        The radar will loop through the antennas and request data from each antenna at the given
        parameters. It will repeat this process for the given number of repetitions.

        The data is returned as a ctypes array. The function waits for the radar to respond until timeout is reached
        and then will exit.

        Inputs:
            antenna_numbers(list) - list of antenna numbers
            repetitions(int) - number of repetitions
            r_start(float) - start of the frame area
            start_to_stop(float) - end of the frame area
            prf_div(int) - prf divider
            pps(int) - pps
            fps(float) - fps
            downconversion_enabled(int) - 1 if downconversion is enabled, 0 otherwise
        Outputs:
            flag(int) - flag indicating the status of the radar request
            data_ctypes(ctypes.c_float) - data from the radar
        """
        IntArray = ctypes.c_int*len(antenna_numbers)
        antenna_ctypes = IntArray(*antenna_numbers)
        flag = ctypes.c_int(0)
        r_start = r_start + self.r_zero
        nbins = int(x4_calculate_bin_number(r_start, r_start + start_to_stop, downconversion_enabled))
        print(nbins)
        DataFloatArray = ctypes.c_float*(repetitions*nbins*len(antenna_numbers))
        data_ctypes = DataFloatArray(*[0.0]*(repetitions*nbins*len(antenna_numbers)))
        result = self.mylib.radar_request(antenna_ctypes, len(antenna_numbers), repetitions, data_ctypes,
                    r_start, start_to_stop,
                    ctypes.c_uint8(prf_div), ctypes.c_uint16(pps), fps,
                    ctypes.c_uint8(downconversion_enabled), ctypes.byref(flag))
        
        start = time.time()
        while(not flag):
            if time.time() - start > self.timeout:
                print("Radar unresponsive")
                break
            else:
                continue
        return result, data_ctypes, nbins
    
    def start(self):
        """
        Start the radar loop. Continuously checks for new requests and processes them.
        In case of an error, the error is appended to the return json and the callback is called.
        """
        while True:
            if not self.request_queue.empty():
                return_json = {
                    'status': -1,
                    'data': (ctypes.c_float*1)(0.0),
                    'nbins': 1,
                    'errors': [],
                    'repetitions': 1,
                    'antenna_number': 1,
                    'command': 'init'
                } 
                info = self.request_queue.get()
                try:
                    self.process_request(info['data'], info['callback'], return_json)
                except Exception as e:
                    print("Unable to process request: ", e)
                    append_error(return_json, "Unable to process request")
                    info['callback'](return_json)
                finally:
                    # python's queue is thread safe - this indicates the given task is done
                    self.request_queue.task_done()
            time.sleep(0.01)
    
    def process_request(self, data: dict, callback, return_json: dict):
        """
        Process the request. The request is in json format and is processed based on the command.
        The command can be 'init' or 'request'. The parameters are checked and the appropriate
        function is called.
        Inputs:
            data(dict) - json data
            callback(function) - callback function to be called when the result is available
        
        """

        init_params = {key: data.get(key, None) for key in self.init_param_names}
        dynamic_param_names = {key: data.get(key, None) for key in self.dynamic_param_names}
        func = data['command']
        result = -1
        print('Processing Request {}'.format(data))
        if func == 'init':
            if None in init_params.values():
                print("Invalid parameters!")
                append_error(return_json, "Invalid parameters!")
            else:
                result = self.initial_setup(**init_params)
                print(result)
                return_json['status'] = result
                return_json['command'] = func
        elif func == 'request':
            if None in dynamic_param_names.values():
                print("Invalid parameters!")
                append_error(return_json, "Invalid parameters!")
            else:
                status, data, bins = self.radar_request(**dynamic_param_names)
                return_json['status'] = status
                return_json['data'] = data
                return_json['nbins'] = bins
                return_json['repetitions'] = dynamic_param_names['repetitions']
                return_json['antenna_number'] = len(dynamic_param_names['antenna_numbers'])
                return_json['command'] = func
        elif func == 'change_params':
            if None in init_params.values():
                print("Invalid parameters!")
                append_error(return_json, "Invalid parameters!")
            else:
                result = self.change_params(**init_params)
                print(result)
                return_json['status'] = result
                return_json['command'] = func
        else:
            print("Invalid command!")
            append_error(return_json, "Invalid command!")
        callback(return_json)

    def register_request(self, data, callback):
        """
        Register the request in the request queue
        Inputs:
            data(str) - json data
            callback(function) - callback function to be called when the result is available
        """
        if isinstance(data, str):
            try:
                data = json.loads(data)
            except (json.JSONDecodeError, TypeError) as e:
                # here won't close the socket but will simply return the error
                print("Error decoding input json: ", e)
                callback("Error decoding input json!")
                raise json.JSONDecodeError
        elif not isinstance(data, dict):
            raise TypeError('Unknown type')
        info = {'data': data, 'callback': callback}
        self.request_queue.put(info)
        

class RadarCommunication:
    
    def __init__(self, api_class):
        """
        Base class template for the radar communication. The idea would be to have
        some connection type (eg socket, local server - potentially a mix eventually).
        Each type will accept connections to the python application in its own way and pass input 
        parameters using json. The input json is added to the PythonRadarApi class (which
        will be passed in during class instantion) to a queue managed by the class.
        Then the requests will be answered sequentially. I think I'll return the value
        directly to the requesting class - but it'll need to pass a callback method for 
        the Api class to call when processing is done

        Inputs:
            api_class(PythonRadarApi): instance of the radar api class
        """
        self.api_class = api_class
        self.shutdown_event = threading.Event()
        self.shutdown_event.clear()
    
    def setup(self):
        """
        Sets up the communication module
        """
        return self

    def connect(self):
        """
        connects to the client
        """
        return self
    
    def disconnect(self):
        """
        disconnects from client and cleans up
        """
        return self
      
    def result_ready(self, return_json: dict):
        """
        callback to call when processing is finished.
        Default is an a json object to the client
        Inputs:
            return_json (dict): The dictionary of results from processing
        """
        # the data is return as a ctype array - convert to list
        return_json['data'] = list(return_json['data'])
        try:
            self.send(json.dumps(return_json).encode('utf-8'))
        except Exception as e:
            print("Error encoding input json: ", e)
    
    def send(self, json_output: str):
        """
        send the data to the client
        Inputs:
            json_output (str): The serialized json to be send to client
        """
        # to be implemented in base class
        raise NotImplementedError


class RadarSocketCommunication(RadarCommunication):

    def __init__(self, api_class: PythonRadarApi, timeout: float=20, total_empty_count: int=60):
        """
        Class for socket communication Python api
        Inputs:
            api_class(PythonRadarApi) - instance of the radar api class
            host(str) - host ip address
            port(int) - port number
            timeout(float) - timeout for the socket connection
        """
        super().__init__(api_class)
        self.timeout = timeout
        self.total_empty_count = total_empty_count
        self.server_socket = None
        self.client_socket = None
        self.client_addr = None
        self.empty_count = 0
    
    def setup(self, host: str='0.0.0.0', port: int=5044):
        """
        Setup the server socket
        """
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.bind((host, port))
        self.server_socket.listen(5)
        
    def connect(self):  
        """
        Connect to the client
        """
        readable, _, _ = select.select([self.server_socket], [], [], 1)
        if self.server_socket in readable:
            self.client_socket, self.addr = self.server_socket.accept()
         
    def request(self):
        """
        Loop to receive the client requests and pass it to the api class
        """
        # TODO - if the client doesn't close properly, it can send empty data for a period. Not a huge deal since the Json won't be parsed
        # properly and the exception is handled, but may want to add a condition to exit upon receiving empty data
        while self.client_socket is not None:
            try:                
                data = self.client_socket.recv(1024).decode('utf-8')
                print("Received data: ", data)
                if not data:
                    self.empty_count += 1
                    print(self.empty_count)
                    if self.empty_count >= self.total_empty_count:
                        print("Too many empty packets. Closing connection")
                        self.reset_socket()
                else:
                    self.api_class.register_request(data, self.result_ready)               
            except socket.timeout as e:
                print("Connection timed out!")
                self.reset_socket()
                time.sleep(10)
            except Exception as e:
                print("Error: ", e)
                # is there a point in sending an error?
                self.reset_socket()
                time.sleep(10)
            time.sleep(0.1)
    
    def reset_socket(self):
        self.client_socket.close()
        self.client_socket = None
        self.empty_count = 0     
    
    def start(self):
        """
        Start the communication loop. This listens for incoming connections and responds
        to requests from the connections. If the current client connection is closed, it
        listens for new connections.
        """
        while not self.shutdown_event.is_set():
            self.connect()
            self.request()
            time.sleep(0.1)
        print("Thread event received. Exiting")
    
    def disconnect(self):
        """
        Close the sockets
        """
        if self.client_socket is not None:
            self.client_socket.close()
            self.client_socket = None
        if self.server_socket is not None:
            self.server_socket.close()
            self.server_socket = None
        self.shutdown_event.set()

    def result_ready(self, return_data: dict):
        """
        Callback to call when processing is finished.
        The data will be returned to the client in bytes with the following format:
        4 bytes for the length of the data
        4 bytes for the status (int)
        4 bytes for the nbins (int)
        4 bytes for the number of repetitions (int)
        4 bytes for the number of antennas (int)
        n*4 bytes for the data (float)
        Inputs:
            return_data (dict): The dictionary of results from processing
        """
        try:
            data = return_data['data']
            ctype_bytes = bytes(data)
            nbins = return_data['nbins']
            repetitions = return_data['repetitions']
            antenna_number = return_data['antenna_number']
            status = return_data['status']
            packed_data = struct.pack('iiii', status, nbins, repetitions, antenna_number) + ctype_bytes
            self.send(packed_data)
        except Exception as e:
            print("Error encoding input json: ", e)
    
    def send(self, json_output: bytes):
        """
        Send the json output to the client
        Inputs:
            json_output(bytes) - json output to be sent
        """
        try:
            data_length = len(json_output)
            self.client_socket.sendall(data_length.to_bytes(4, 'big'))
            self.client_socket.sendall(json_output)
        except Exception as e:
            # how to handle this exception?
            print("Error sending data: ", e)
            self.client_socket.close()
            self.client_socket = None

class RadarFlaskCommunication(RadarCommunication):
    """
    Runs a lightweight local server and responds to http requests
    """
    def __init__(self, api_class, radar_thread):
        """
        Sets up a server to receive requests and returns results to a specified address
        Inputs:
            api_class (PythonRadarApi): instance of the radar api
            radar_tread (threading.Thread): thread instance to check thread operation
        """
        super().__init__(api_class)
        self.radar_thread = radar_thread
    
    def setup(self):
        """
        sets up the flask app
        """
        self.app = Flask(__name__)
        self.setup_routes()
        self.setup_error()
         
    def start(self, host: str='0.0.0.0', port=5043):
        """
        runs the flask app
        """
        self.app.run(host=host, port=port, debug=True)
    
    def setup_routes(self):
        """
        sets up the route for radar requests and passes to self.api_class
        """
        @self.app.route('/radar', methods=['POST'])
        def radar_request():
            data = request.get_json()
            print("Received data: ", data)
            try:
                data = json.loads(data)
            except (json.JSONDecodeError, TypeError) as e:
                # here won't close the socket but will simply return the error
                print("Error decoding input json: ", e)
                # forward json to radar to get complete error message uploaded
                self.api_class.register_request(data, self.result_ready)
                # TODO: add handling for this error to raise the error as in socket?
                return jsonify({'message': 'Invalid JSON'}), 400
            
            self.return_address = data['return_address']
            # TODO: move someplace else
            if not self.radar_thread.is_alive():
                raise RuntimeError("Radar thread has stopped!")
            self.api_class.register_request(data, self.result_ready)
            return jsonify({'message': 'Request received'}), 200

    def setup_error(self):
        """
        handles runtime errors
        """
        def handle_runtime_error(self, error):
            print("Error: ", error)
            response = jsonify({'error': str(error)})
            response.status_code = 500
            return response
    
    def result_ready(self, return_data: dict):
        """
        Callback to call when processing is finished.
        The data will be returned to the client in bytes with the following format:
        4 bytes for the length of the data
        4 bytes for the status (int)
        4 bytes for the nbins (int)
        4 bytes for the number of repetitions (int)
        4 bytes for the number of antennas (int)
        n*4 bytes for the data (float)
        Inputs:
            return_data (dict): The dictionary of results from processing
        """
        try:
            print(return_data)
            data = return_data.pop('data')
            ctype_bytes = bytes(data)
            self.send(ctype_bytes, return_data)
        except Exception as e:
            print("Error encoding input json: ", e)

    def send(self, binary_output: bytes, json_output: dict):
        """
        sends a multipart form post to the endpoint. The JSON is sent as metadata in the first part, tagged as application/json.
        The data is sent in the second part tagged as application/octet-stream. There is a unique boundary strin (uuid) to separate the parts.
        """
        try:
            body, content_type = create_multipart_form_data(binary_output, json.dumps(json_output))
        except (json.JSONDecodeError, TypeError) as e:
            print("Error encoding input json: ", e)
            req = urllib.request.Request(self.return_address, data=json.dumps({'error': 'Cannot encode json input'}).encode('utf-8'), headers={"Content-Type": "application/json"})
        except Exception as e:
            print("Error creating response: ", e)
            req = urllib.request.Request(self.return_address, data=json.dumps({'error': 'Unknown error', 'details': e}).encode('utf-8'), headers={"Content-Type": "application/json"})
        else:
            req = urllib.request.Request(self.return_address, data=body,
                                        headers={"Content-Type": content_type, "Content-Length": str(len(body))},
                                        method='POST')
        # Send the request and get the response
        with urllib.request.urlopen(req) as response:
            response_data = response.read()
            print('Response from client')
            print(response.getcode())
            print(response_data.decode())
    

def create_multipart_form_data(binary_data: bytes, json_data: str, json_field:str = 'json',
                               binary_field='binary') -> Tuple[bytes, str]:
    boundary = uuid.uuid4().hex
    content_type = 'multipart/form-data; boundary={}'.format(boundary)
    body = (
        '--{}\r\n'.format(boundary) +
        'Content-Disposition: form-data; name="{}"; filename="metadata.json"\r\n'.format(json_field) +
        'Content-Type: application/json\r\n\r\n' + 
        '{}\r\n'.format(json_data) +
        '--{}\r\n'.format(boundary) +
        'Content-Disposition: form-data; name="{}"; filename="data.bin"\r\n'.format(binary_field) +
        'Content-Type: application/octet-stream\r\n\r\n'
    ).encode('utf-8') + binary_data + '\r\n--{}--\r\n'.format(boundary).encode('utf-8')
    return body, content_type
    

def append_error(return_json, error):
    return_json['errors'].append(error)

def start_flask_server():
    """
    runs the flask server
    """
    # thread for radar
    radar_instance = PythonRadarApi()
    radar_thread = threading.Thread(target=radar_instance.start, daemon=True)
    radar_thread.start()
    server_inst = RadarFlaskCommunication

    # loop for communication.
    server_comm = server_inst(radar_instance, radar_thread)
    server_comm.setup()
    # flask runs on the main thread (in debug server)
    # here if the program is ended it will enter the exception
    # How can I monitor the radar_thread? For now I've moved it inside the flask server - but can see if there are better options
    try:
        server_comm.start()
    except (KeyboardInterrupt, SystemExit):
        print("Program stopped")
        print("Exiting")
    except Exception as e:
        print("Unexpected error: ", e)
        print("Exiting")
    finally:  
        # wait for radar thread to finish
        print("Waiting for all requests to finish")
        radar_instance.request_queue.join()
        print("Exiting")
        sys.exit(0)

def start_socket_server():
    """
    runs the socket server
    """
    # thread for radar
    radar_instance = PythonRadarApi()
    radar_thread = threading.Thread(target=radar_instance.start, daemon=True)
    radar_thread.start()
    server_inst = RadarSocketCommunication

    # loop for communication.
    server_comm = server_inst(radar_instance)
    server_comm.setup()
    server_thread = threading.Thread(target=server_comm.start)
    server_thread.start()
    try:
        while True: 
            # monitor the threads
            if not radar_thread.is_alive():
                print("Radar thread has stopped! Exiting loop")
                break
            if not server_thread.is_alive():
                print("Socket thread has stopped! Exiting loop")
                break 
            time.sleep(0.5)
    except (KeyboardInterrupt, SystemExit):
        print("Program stopped")
        print("Exiting")
    except Exception as e:
        print("Unexpected error: ", e)
        print("Exiting")  
    finally:  
        server_comm.disconnect()
        print('Waiting for socket thread')
        server_thread.join()
        # wait for radar thread to finish
        print("Waiting for all requests to finish")
        radar_instance.request_queue.join()
        print("Exiting")
        sys.exit(0)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Python Api for X4')
    parser.add_argument('--flask', action="store_true", help='Enable flask server')
    args = parser.parse_args()
    if args.flask and flask_flag:
        print("Starting Flask server")
        start_flask_server()
    else:
        print("Starting socket server")
        start_socket_server()
