import datetime
import json

import numpy as np
import os
import time
import threading
from radar_client import RadarSocketClient, ChunkDispatcher
from concurrent.futures import ThreadPoolExecutor
import math


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
    'command': 'request', 'antenna_numbers': [56], 'repetitions': 15000,
    'r_start': 0.7, 'start_to_stop': 2.5,
    'prf_div': 8, 'pps': 7, 'fps': 350.0, 'downconversion_enabled': 0, 'chunk_size': 3000
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


def process_specific_chunk(dispatcher, chunk_index, timeout=60):
    """
    Demo for function that can be run in separate threads to process specific chunks
    """
    try:
        # the chunks are indexed and each thread waits for its index
        chunk_data, metadata = dispatcher.wait_for_chunk(chunk_index, timeout=timeout)
        print(f"Thread {threading.current_thread().name} processing chunk {chunk_index}")
        # Process chunk_data here
        # ...
        return chunk_data, metadata
    except TimeoutError:
        print(f"Timeout waiting for chunk {chunk_index}")
        return None, None
    


if __name__ == "__main__":
    server_ip = '192.168.1.232'
    #server_ip = '10.0.0.19'
    upload_folder = '/Users/leokatz/Documents/RadarApi/data/'
    file_name = 'test_data'
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
    
    chunk_request = True
    if not chunk_request:
        params_request = params_request1
        for i in range(1):
            print("Timestamp of request: ", datetime.datetime.now())
            data, output_dict = radar_client.request(params_request)
            start = time.time()
            print("Timestamp of start: ", output_dict['timestamp'])
            print("Time to receive data: ", time.time() - start) 
            print("Status: ", output_dict['status'])
            print("Nbins: ", output_dict['nbins'])
            print("Repetitions: ", output_dict['repetitions'])
            print("Antenna Number: ", output_dict['antenna_number'])
            print("Chunk Index: ", output_dict['chunk_index'])
            print("Total Chunks: ", output_dict['total_chunks'])
            if not os.path.exists(upload_folder):
                os.makedirs(upload_folder)
            folder_path = os.path.join(upload_folder, datetime.datetime.now().strftime("%Y_%m_%d"))
            if not os.path.exists(folder_path):
                os.makedirs(folder_path)
            # put file name convention here
            save_file_name = file_name + '_' + datetime.datetime.now().strftime("%H_%M_%S") + '.npy'
            np.save(os.path.join(folder_path, save_file_name), data)    
            print(data.shape)
    else:
        all_chunks = {}
        dispatcher = radar_client.request_with_dispatcher(params_request1)
        if params_request1['chunk_size'] > 0:
            total_chunks = math.ceil(params_request1['repetitions']*len(params_request1['antenna_numbers']) / params_request1['chunk_size'])
        else:
            total_chunks = 1

        # Process chunks with thread pool
        with ThreadPoolExecutor(max_workers=4) as executor:
            # Submit tasks for specific chunks
            futures = {
                executor.submit(process_specific_chunk, dispatcher, i): i
                    for i in range(total_chunks)  # Example: process first 10 chunks
            }
            
            # Wait for all tasks to complete
            for future in futures:
                chunk_index = futures[future]
                try:
                    chunk_data, metadata = future.result()
                    if chunk_data is not None:
                        all_chunks[chunk_index] = (chunk_data, metadata)
                except Exception as e:
                    print(f"Error processing chunk {chunk_index}: {e}")
        
        if all_chunks:
            # Get metadata from first chunk
            first_metadata = all_chunks[0][1]
            
            # Combine all chunk data
            combined_data = np.concatenate([all_chunks[i][0] for i in sorted(all_chunks.keys())])
            
            # Save combined data
            if not os.path.exists(upload_folder):
                os.makedirs(upload_folder)
            folder_path = os.path.join(upload_folder, datetime.datetime.now().strftime("%Y_%m_%d"))
            if not os.path.exists(folder_path):
                os.makedirs(folder_path)
            
            save_file_name = file_name + '_' + datetime.datetime.now().strftime("%H_%M_%S") + '.npy'
            np.save(os.path.join(folder_path, save_file_name), combined_data)
            
            print(f"Saved combined data with shape: {combined_data.shape}")
            print(f"Total chunks processed: {len(all_chunks)}")
        
    radar_client.disconnect()