import datetime
import json
import socket
import numpy as np
import os
import time

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

if __name__ == "__main__":
    server_ip = '192.168.1.232'
    upload_folder = './data'
    file_name = 'data'

    client_socket = create_socket()
    client_socket.connect((server_ip, 5044))
    # Only call init once per session!
    init = False
    change_params = False
    if init:      
        data = json.dumps(params_init)
        client_socket.send(data.encode('utf-8'))
        response = receive_data(client_socket)
        response = response.decode('utf-8')
        print("Response: ", response)

    if change_params:      
        data = json.dumps(params_change)
        client_socket.send(data.encode('utf-8'))
        response = receive_data(client_socket)
        response = response.decode('utf-8')
        print("Response: ", response)
    
    data = json.dumps(params_request2)
    
    client_socket.send(data.encode('utf-8'))
    start = time.time()
    response = receive_data(client_socket).decode('utf-8') 
    print("Time to receive data: ", time.time() - start) 
    start = time.time()
    response = json.loads(response)
    print("Time to json convert: ", time.time() - start)
    data = response['data']
    nbins = response['nbins']
    if not os.path.exists(upload_folder):
        os.makedirs(upload_folder)
    folder_path = os.path.join(upload_folder, datetime.datetime.now().strftime("%Y_%m_%d"))
    if not os.path.exists(folder_path):
        os.makedirs(folder_path)
    # put file name convention here
    file_name = file_name + '_' + datetime.datetime.now().strftime("%H_%M_%S") + '.npy'
    # np.save(os.path.join(folder_path, file_name), np.array(data).reshape((params_request['repetitions'], len(params_request['antenna_numbers']), nbins)).squeeze())
    print("done")

    data = json.dumps(params_request2)
    
    client_socket.send(data.encode('utf-8'))
    start = time.time()
    response = receive_data(client_socket).decode('utf-8')  
    print("Time to receive data: ", time.time() - start)
    start = time.time()
    response = json.loads(response)
    print("Time to json convert: ", time.time() - start)
    data = response['data']
    nbins = response['nbins']
    if not os.path.exists(upload_folder):
        os.makedirs(upload_folder)
    folder_path = os.path.join(upload_folder, datetime.datetime.now().strftime("%Y_%m_%d"))
    if not os.path.exists(folder_path):
        os.makedirs(folder_path)
    # put file name convention here
    file_name = file_name + '_' + datetime.datetime.now().strftime("%H_%M_%S") + '.npy'
    # np.save(os.path.join(folder_path, file_name), np.array(data).reshape((params_request['repetitions'], len(params_request['antenna_numbers']), nbins)).squeeze())
    print("done")
    try:
        while True:
            pass
    except KeyboardInterrupt:
        print("Exiting...")
        client_socket.close()