# -*- coding: utf-8 -*-
"""
Created on Sat Mar 27 10:48:56 2021

@author: nanashi
"""

from PIL import Image
import numpy as np
from copy import copy


# converts char from file to numpy greyscale array
def char_to_numpy(char_data, offset):
    
    blk_pattern = [0, 2, 6, 1, 3, 7, 4, 5, 8]                                   # 3x3 block decode pattern
    pix_pattern = [0, 2, 8, 10, 1, 3, 9, 11, 4, 6, 12, 14, 5, 7, 13, 15]        # 4x4 pixel decode pattern
    
    char_data = char_data[offset:offset+144]
    img_raw = np.zeros((12,12), dtype=np.uint8)
    for i in range(0, 3):                                                       # block y-coord
        for j in range(0, 3):                                                   # block x-coord
            for k in range(0, 4):                                               # pixel y-coord
                for m in range(0, 4):                                           # pixel x-coord
                    idx = blk_pattern[i*3+j]*16
                    idx_off =  pix_pattern[k*4+m]
                    idx += idx_off
                    img_raw[i*4+k,j*4+m] = char_data[idx]
                    
    return img_raw
    

with open("FONT.CG", "rb") as ifile:
    char_data = ifile.read()    

placeholder = np.zeros((12,12), dtype=np.uint8)                                 # black placeholder

num_chars = int(len(char_data)/144) - 144                                            # number of characters in FONT.CG
height = 114
width = 32

# Decode all characters from FONT.CG and convert them to a png 
# with a height x width block grid
v_char = []
for i in range(0, height):
    h_char = []
    for j in range(0, width):
        char_num = i*width+j
        if char_num < num_chars:
            h_char.append(char_to_numpy(char_data, char_num*144))
        else:
            h_char.append(placeholder)
    v_char.append(copy(h_char))
        
v_char = [np.hstack(line) for line in v_char]
img_raw = np.vstack(v_char)

PIL_image = Image.fromarray(img_raw, "L")
PIL_image.save("decoded_fw.png")
