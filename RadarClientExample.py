import datetime
import json
import socket
import numpy as np
import os
import time
import struct

params_init = {
    'command': 'init', 'frequency': 'US', 'transmit_gain': 'HIGH', 'dac_min': 969, 'dac_max': 1100,
    'tzero_ns': 1.333, 'iterations': 64
    }

params_change = {
    'command': 'change_params', 'frequency': 'US', 'transmit_gain': 'HIGH', 'dac_min': 969, 'dac_max': 1100,
    'tzero_ns': 1.333, 'iterations': 64
    }

# FPS 10 Respiration
params_request1 = {
    'command': 'request', 'antenna_numbers': [56], 'repetitions': 800,
    'r_start': 0.5, 'start_to_stop': 2.5,
    'prf_div': 8, 'pps': 250, 'fps': 10.0, 'downconversion_enabled': 0
}

# FPS 70 Respiration
params_request2 = {
    'command': 'request', 'antenna_numbers': [56], 'repetitions': 4200,
    'r_start': 0.5, 'start_to_stop': 2.5,
    'prf_div': 8, 'pps': 35, 'fps': 70.0, 'downconversion_enabled': 0
}

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
    data = np.frombuffer(received_data[16:], np.float32)
    return status, nbins[0], repetitions[0], antenna_num[0], data

if __name__ == "__main__":
    server_ip = '192.168.1.232'
    # server_ip = '192.168.252.10'
    upload_folder = './data'
    file_name = 'data'
    
    client_socket = create_socket()
    client_socket.connect((server_ip, 5044))
    # Only call init once per session!
    init = True
    change_params = False
    if init:      
        data = json.dumps(params_init)
        client_socket.send(data.encode('utf-8'))
        response = receive_data(client_socket)
        status, nbins, repetitions, antenna_num, data = unpack_data(response)
        print("Status: ", status)

    if change_params:      
        data = json.dumps(params_change)
        client_socket.send(data.encode('utf-8'))
        response = receive_data(client_socket)
        status, nbins, repetitions, antenna_num, data = unpack_data(response)
        print("Status: ", status)
    
    params_request = params_request2
    params_to_send = json.dumps(params_request)  
    for i in range(2):
        client_socket.send(params_to_send.encode('utf-8'))
        start = time.time()
        response = receive_data(client_socket)
        status, nbins, repetitions, antenna_num, data = unpack_data(response)
        print("Time to receive data: ", time.time() - start) 
        print("Status: ", status)
        print("Nbins: ", nbins)
        print("Repetitions: ", repetitions)
        print("Antenna Number: ", antenna_num)
        if not os.path.exists(upload_folder):
            os.makedirs(upload_folder)
        folder_path = os.path.join(upload_folder, datetime.datetime.now().strftime("%Y_%m_%d"))
        if not os.path.exists(folder_path):
            os.makedirs(folder_path)
        # put file name convention here
        save_file_name = file_name + '_' + datetime.datetime.now().strftime("%H_%M_%S") + '.npy'
        np.save(os.path.join(folder_path, save_file_name), data.reshape((params_request['repetitions'], len(params_request['antenna_numbers']), nbins)).squeeze())
        print("done loop ", i)

    # TODO - add a specific code to send to server to close the connection
    client_socket.close()