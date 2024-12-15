import numpy as np
import select
import socket
import json
import struct
from queue import Queue
from threading import Event
import threading

class ChunkDispatcher:
    def __init__(self):
        self.chunks = {}  # Dictionary to store chunks by index
        self.chunk_events = {}  # Events to signal chunk availability
        self.lock = threading.Lock()

    def set_chunk(self, chunk_index: int, chunk_data, metadata: dict):
        """Called by generator when new chunk arrives"""
        with self.lock:
            self.chunks[chunk_index] = (chunk_data, metadata)
            # Create and set event if someone is waiting for this chunk
            if chunk_index in self.chunk_events:
                self.chunk_events[chunk_index].set()

    def wait_for_chunk(self, chunk_index: int, timeout=None):
        """Called by consumer threads to wait for specific chunks"""
        with self.lock:
            # If chunk already received, return it immediately
            if chunk_index in self.chunks:
                return self.chunks[chunk_index]
            
            # Create event to wait for this chunk
            if chunk_index not in self.chunk_events:
                self.chunk_events[chunk_index] = Event()
        
        # Wait for chunk to arrive
        if self.chunk_events[chunk_index].wait(timeout):
            return self.chunks[chunk_index]
        else:
            raise TimeoutError(f"Timeout waiting for chunk {chunk_index}")


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
    def build_output(status: int, nbins: int, repetitions: int, antenna_number: int, timestamp:str, chunk_index: int, total_chunks: int, downconversion_enabled: bool, errors: str = None, **kwargs) -> dict:
        """
        Utility to build the output dictionary
        Inputs:

        """
        output_dict = dict(status=status, nbins=nbins, repetitions=repetitions, chunk_index=chunk_index, total_chunks=total_chunks,
                            antenna_number=antenna_number, timestamp=timestamp, errors=errors, downconversion_enabled=downconversion_enabled)
        return output_dict

    def request(self, params: dict) -> dict:
        """
        Initiates a request to the radar. All data is returned at end of request
        Inputs:
            params(dict): The parameters for radar request
        """
        data = json.dumps(params)
        self.client_socket.send(data.encode('utf-8'))
        all_data = []
        metadata = None
        while True:
            metadata_bytes = receive_data(self.client_socket)
            chunk_metadata = json.loads(metadata_bytes.decode('utf-8'))
            if metadata is None:
                metadata = chunk_metadata
            else:
                metadata['chunk_index'] = chunk_metadata['chunk_index']
            
            chunk = receive_data(self.client_socket)
            chunk_data = np.frombuffer(chunk, dtype=np.float32)
            all_data.append((metadata['chunk_index'], chunk_data))

            if metadata['total_chunks'] == metadata['chunk_index'] + 1:
                break
        
        sorted_data = sorted(all_data, key=lambda x: x[0])
        data = np.concatenate([item[1] for item in sorted_data])
        if metadata['downconversion_enabled']:
            data = data.reshape(-1, metadata['antenna_number'], 2*metadata['nbins']).squeeze()
            data = data[:, :, :metadata['nbins']] + 1j * data[:, :, metadata['nbins']:]
        else:

            data = data.reshape(-1, metadata['antenna_number'], metadata['nbins']).squeeze()

        output_dict = self.build_output(**metadata, data=data)
        return data, output_dict

    def request_with_dispatcher(self, params: dict) -> ChunkDispatcher:
        """
        Returns a dispatcher that can be used to wait for specific chunks
        """
        dispatcher = ChunkDispatcher()
        
        # Start generator in separate thread
        def generator_thread():
            for chunk_data, metadata in self.request_generator(params):
                print(f"Received chunk {metadata['chunk_index']} for dispatch")
                dispatcher.set_chunk(metadata['chunk_index'], chunk_data, metadata)
        
        thread = threading.Thread(target=generator_thread)
        thread.daemon = True
        thread.start()
        
        return dispatcher

    def request_generator(self, params: dict):
        """
        Generator that yields chunks as they arrive

        """
        data = json.dumps(params)
        self.client_socket.send(data.encode('utf-8'))
        
        while True:
            metadata_bytes = receive_data(self.client_socket)
            metadata = json.loads(metadata_bytes.decode('utf-8'))
            
            chunk = receive_data(self.client_socket)
            chunk_data = np.frombuffer(chunk, dtype=np.float32)
            
            if metadata['downconversion_enabled']:
                shaped_data = chunk_data.reshape(-1, metadata['antenna_number'], 2*metadata['nbins']).squeeze()
                shaped_data = shaped_data[:, :, :metadata['nbins']] + 1j * shaped_data[:, :, metadata['nbins']:]
            else:
                shaped_data = chunk_data.reshape(-1, metadata['antenna_number'], metadata['nbins']).squeeze()
            
            yield shaped_data, metadata
            
            if metadata['total_chunks'] == metadata['chunk_index'] + 1:
                break

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
    remaining_length = data_length
    while remaining_length > 0:
        chunk = client_socket.recv(min(remaining_length, 1024*64))
        if not chunk:
            print("No data received")
            break
        received_data += chunk
        remaining_length -= len(chunk)
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