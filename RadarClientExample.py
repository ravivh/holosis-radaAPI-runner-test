import datetime
import json

import numpy as np
import os
import time

from radar_client import RadarSocketClient


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
    'r_start': 0.7, 'start_to_stop': 2.5,
    'prf_div': 8, 'pps': 35, 'fps': 70.0, 'downconversion_enabled': 0
}

params_request_posture = {
    'command': 'request', 'antenna_numbers': list(range(64)), 'repetitions': 10,
    'r_start': 0.7, 'start_to_stop': 2.5,
    'prf_div': 16, 'pps': 5, 'fps': 250.0, 'downconversion_enabled': 0
}

params_request_motion = {
    'command': 'request', 'antenna_numbers': [56], 'repetitions': 560,
    'r_start': 0.7, 'start_to_stop': 2.5,
    'prf_div': 10, 'pps': 4, 'fps': 350.0, 'downconversion_enabled': 0
}


if __name__ == "__main__":
    #server_ip = '192.168.1.232'
    server_ip = '192.168.252.12'
    upload_folder = './data'
    file_name = 'posture_data'
    # time.sleep(20)
    radar_client = RadarSocketClient(server_ip, 5044)
    radar_client.connect()
    # Only call init once per session!
    init = True
    change_params = False
    if init:      
        output_dict = radar_client.init(params_init)
        print("Status: ", output_dict['status'])

    if change_params:      
        response = radar_client.change_params(params_change)
        print("Status: ", response['status'])
    
    params_request = params_request_posture
    for i in range(2):
        print("Timestamp of request: ", datetime.datetime.now())
        data, output_dict = radar_client.request(params_request)
        start = time.time()
        print("Timestamp of start: ", output_dict['timestamp'])
        print("Time to receive data: ", time.time() - start) 
        print("Status: ", output_dict['status'])
        print("Nbins: ", output_dict['nbins'])
        print("Repetitions: ", output_dict['repetitions'])
        print("Antenna Number: ", output_dict['antenna_num'])
        if not os.path.exists(upload_folder):
            os.makedirs(upload_folder)
        folder_path = os.path.join(upload_folder, datetime.datetime.now().strftime("%Y_%m_%d"))
        if not os.path.exists(folder_path):
            os.makedirs(folder_path)
        # put file name convention here
        save_file_name = file_name + '_' + datetime.datetime.now().strftime("%H_%M_%S") + '.npy'
        print(data.shape)
        
    radar_client.disconnect()