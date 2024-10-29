import numpy as np
import select
import socket
import json
import struct

class RadarSocketClient:

    def __init__(self, server_ip: str, port: int):
        """
        Class for initializing a client socket with the radar.
        Inputs:
            server_ip (str): The ip of the server
            port (int): port the server is listening on
        """
        self.server_ip = server_ip
        self.port = port
        self.client_socket = None
    
    def connect(self):
        """
        Initializes a socket connection
        """
        self.client_socket = create_socket()
        self.client_socket.connect((self.server_ip, self.port))
    
    def disconnect(self):
        """
        Disconnects the socket
        """
        self.client_socket.close()
    
    def init(self, params_init: dict):
        """
        Initializes the radar
        Inputs:
            params_init (dict): the dictionary of initialization params
        """
        params_init['command'] = 'init'
        _, output_dict = self.request(params_init)
        return output_dict
    
    def change_params(self, params_change: dict):
        """
        Changes the radar parameters
        Inputs:
            params_change (dict): the dictionary of parameters to change
        """
        params_change['command'] = 'change_params'
        _, output_dict = self.request(params_change)
        return output_dict
    
    def clear_buffer(self):
        self.client_socket.setblocking(False)  # Set socket to non-blocking mode
        try:
            while True:
                ready = select.select([self.client_socket], [], [], 1)  # Check for readability
                if ready[0]:
                    self.client_socket.recv(1024)
                else:
                    break  # No more data to read
        except socket.error as e:
            print(f"Socket error: {e}")
        finally:
            self.client_socket.setblocking(True)  # Set socket back to blocking mode
            print("Buffer has been cleared")

    @staticmethod
    def build_output(status: int, nbins: int, repetitions: int, antenna_num: int, date_str:str) -> dict:
        """
        Utility to build the output dictionary
        Inputs:

        """
        output_dict = dict(status=status, nbins=nbins, repetitions=repetitions,
                            antenna_num=antenna_num, timestamp=date_str)
        return output_dict

    def request(self, params: dict) -> dict:
        """
        Initiates a request to the radar
        Inputs:
            params(dict): The parameters for radar request
        """
        data = json.dumps(params)
        self.client_socket.send(data.encode('utf-8'))
        response = receive_data(self.client_socket)
        # TODO: add command and errors to socket
        status, nbins, repetitions, antenna_num, data, date_str = unpack_data(response)
        output_dict = self.build_output(status, nbins, repetitions, antenna_num, date_str)
        return data, output_dict

def create_socket():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 64*1024)
    return client_socket

def receive_data(client_socket):
    data_length = client_socket.recv(4)
    if not data_length:
        print("No data received")
    data_length = int.from_bytes(data_length, byteorder='big')
    received_data = b''
    while len(received_data) < data_length:
        chunk = client_socket.recv(1024*64)
        if not chunk:
            print("No data received")
            break
        received_data += chunk
    return received_data

def unpack_data(received_data):
    status = struct.unpack('i', received_data[:4])
    nbins = struct.unpack('i', received_data[4:8])
    repetitions = struct.unpack('i', received_data[8:12])
    antenna_num = struct.unpack('i', received_data[12:16])
    downconversion = struct.unpack('i', received_data[16:20])
    date_length = struct.unpack('i', received_data[20:24])[0]
    date_str = received_data[24:24+date_length].decode('utf-8')
    
    if downconversion[0] == 1:
        data = np.frombuffer(received_data[24 + date_length:], np.float32).reshape(-1, antenna_num[0], 2*nbins[0]).squeeze()
        data = data[:, :, :nbins[0]] + 1j * data[:, :, nbins[0]:]
    else:
        data = np.frombuffer(received_data[24 + date_length:], np.float32).reshape(-1, antenna_num[0], nbins[0]).squeeze()
    
    return status[0], nbins[0], repetitions[0], antenna_num[0], data, date_str