import json
import os
import requests
import numpy as np
import struct

params_init = {
    'command': 'init', 'frequency': 'US', 'transmit_gain': 'HIGH', 'dac_min': 969, 'dac_max': 1100,
    'tzero_ns': 1.333, 'iterations': 64
    }

params_change = {
    'command': 'change_params', 'frequency': 'US', 'transmit_gain': 'HIGH', 'dac_min': 969, 'dac_max': 1100,
    'tzero_ns': 1.333, 'iterations': 64
    }

# posture
params_request = {
    'command': 'request', 'antenna_numbers': list(range(64)), 'repetitions': 60,
    'r_start': 0.5, 'start_to_stop': 2.5,
    'prf_div': 16, 'pps': 5, 'fps': 250.0, 'downconversion_enabled': 0
}

#Motion
#params_request = {
#    'command': 'request', 'antenna_numbers': [56], 'repetitions': 560*4,
#    'r_start': 0.5, 'start_to_stop': 2.5,
#    'prf_div': 10, 'pps': 4, 'fps': 350.0, 'downconversion_enabled': 0
#}

# resp standard
# params_request = {
#     'command': 'request', 'antenna_numbers': [56], 'repetitions': 800,
#     'r_start': 0.5, 'start_to_stop': 2.5,
#     'prf_div': 8, 'pps': 250, 'fps': 10.0, 'downconversion_enabled': 0
# }

# resp 70 fps
params_request = {
    'command': 'request', 'antenna_numbers': [56], 'repetitions': 560,
    'r_start': 0.5, 'start_to_stop': 2.5,
    'prf_div': 8, 'pps': 35, 'fps': 70.0, 'downconversion_enabled': 0
}

def add_upload_folder(data, return_address):
    data['return_address'] = return_address
    return data

def post_to_server(data, server_ip):
    try:
        response = requests.post(server_ip, json=data)
        response.raise_for_status()  # Raise an HTTPError for bad responses (4xx and 5xx)
        print("Status Code:", response.status_code)
        print("Response Text:", response.text)
    except requests.exceptions.HTTPError as errh:
        print("HTTP Error:", errh)
    except requests.exceptions.ConnectionError as errc:
        print("Error Connecting:", errc)
    except requests.exceptions.Timeout as errt:
        print("Timeout Error:", errt)
    except requests.exceptions.RequestException as err:
        print("OOps: Something Else", err)
    print("Response: ", response)


if __name__ == "__main__":
    server_ip = 'http://192.168.252.12:5043/radar'
    return_address = 'http://192.168.252.9:80/uploads'
    init = True
    change_params = False
    if init:      
        data = json.dumps(add_upload_folder(params_init, return_address))
        post_to_server(data, server_ip)

    if change_params: 
        data = json.dumps(add_upload_folder(params_change, return_address))
        post_to_server(data, server_ip)
    
    data = json.dumps(add_upload_folder(params_request, return_address))
    post_to_server(data, server_ip)
