import argparse
import datetime
from flask import Flask, request, jsonify
import numpy as np
import os

app = Flask(__name__)

@app.route('/uploads', methods=['POST'])
def upload_file():
    if request.method == 'POST':
        response = request.get_json()
        if 'status' in response:
            if response['status'] == 0:
                print("Success")
                nbins = response['nbins']
                data = response['data']
                repetitions = response['repetitions']
                antenna_number = response['antenna_number']
                if response['command'] == 'request':
                    
                    if not os.path.exists(app.config['upload_folder']):
                        os.makedirs(app.config['upload_folder'])
                    folder_path = os.path.join(app.config['upload_folder'], datetime.datetime.now().strftime("%Y_%m_%d"))
                    if not os.path.exists(folder_path):
                        os.makedirs(folder_path)
                    # put file name convention here
                    file_name = app.config['file_name'] + '_' + datetime.datetime.now().strftime("%H_%M_%S") + '.npy'
                    # np.save(os.path.join(folder_path, file_name), np.array(data).reshape((repetitions, antenna_number, nbins)).squeeze())
                    print("Data Saved Successfully!!")
                    return jsonify({'message': 'Data saved'}), 200
                else:
                    print("No data return requested")
                    return jsonify({'message': 'Request received'}), 200
            else:
                print("Radar task failed")
                print(response)
                return jsonify({'Error': 'Invalid request'}), 400
        else:
            print("Error")
            print(response)
            return jsonify({'Error': 'Invalid request'}), 400
    else:
        print("Error - invalid route")
        return jsonify({'Error': 'Invalid route'}), 400
        
                
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Flask server for RadarClientExample.py')
    parser.add_argument('--upload_folder', type=str, default='./data', help='Folder to save data')
    parser.add_argument('--file_name', type=str, default='data', help='File name to save data')

    args = parser.parse_args()
    app.config['upload_folder'] = args.upload_folder
    app.config['file_name'] = args.file_name
    server_ip = app.run(host='0.0.0.0', port=80, debug=True)
