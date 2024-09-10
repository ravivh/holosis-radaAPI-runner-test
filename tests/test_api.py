import unittest
import os
import math
import matplotlib.pyplot as plt
import numpy as np

class APITestCase(unittest.TestCase):

    def __init__(self, methodName, old_name, new_name, num_files=10, thresh=0.95, tol=1e-2):
        super(APITestCase, self).__init__(methodName)
        self.old_name = old_name
        self.new_name = new_name
        self.num_files = num_files
        self.data_dir = os.path.join(os.path.dirname(__file__), 'data')
        self.tol = tol
        self.thresh = thresh
    
    def setUp(self):
        
        files = os.listdir(self.data_dir)
        self.old_files = [file for file in files if file.startswith(self.old_name)]
        new_files = [file for file in files if file.startswith(self.new_name)]
        self.new_files = np.concatenate([np.load(os.path.join(self.data_dir, file)) for file in new_files], axis=0)
        self.chosen_files = np.random.choice(range(min(len(self.new_files), len(self.old_files))), self.num_files, replace=False)

    def file_comparison(self):
        """
        compares files from new api to old api with same parameters
        """
        for i in self.chosen_files:
            # new uses 390 bins, old 389
            # also the old doesn't account for bin delay which will impact..
            bin_delay = int(3e8/(1e9*2)*1.333*(389/2.5))
            arr_new = self.new_files[i, :, :-bin_delay-1].squeeze()
            arr_old = np.load(os.path.join(self.data_dir, self.old_files[i]))[:, bin_delay:]
            # Compare the result with the expected output - take x% of the data since there is natural variation
            thresh = np.isclose(arr_new, arr_old, atol=self.tol).sum()/len(arr_new.flatten())
            assert(thresh >= self.thresh)

if __name__ == '__main__':
    
    suite = unittest.TestSuite()
    runner = unittest.TextTestRunner()
    suite.addTest(APITestCase('file_comparison', 'MODE_POSTURE', 'noise_data_posture'))
    suite.addTest(APITestCase('file_comparison', 'MODE_MOTION', 'noise_data_motion', num_files=2))
    runner.run(suite)