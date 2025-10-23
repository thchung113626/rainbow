'''
Created on 2013.08.02

@author: Prog(A)42
'''

import os
import sys
import csv

from PIL import Image as im
 
def overlay_radar_image():
    # print "overlay_radar_image.py starts ......"
    # print
    if len(sys.argv) != 3:
        # print "Not enough arguments"
        sys.exit(1)
    
    shadowFile = "/usr/local/Rainbow5/rainbow/rbMbSystem-x5.7(3rd)/asset/shadow_30.png"

    in_warning = sys.argv[1]
    in_radar_image = sys.argv[2]
        
    # print "Warning msg: " + in_warning
    # print "Radar Image: " + in_radar_image
    # print
    
    overlayFile = os.path.basename(in_warning)
    filename = os.path.splitext(overlayFile)[0]
    timestamp = filename.split("_")[0]
    timestamp = timestamp.replace("T", "")

    output_path = os.path.dirname(in_warning)
    output_path = output_path.replace("/output", "")
    print ("output_path - " + output_path)
    print ("filename - " + filename)
    overlayFile = output_path + "/overlays/" + filename + ".png"
    print ("overlayFile - " + overlayFile)
    outputFile = output_path + "/images/" + timestamp + "00vel.ppi.png" 
            
    # print "Overlay warning on the radar image ......"
    # print "Output: " + outputFile
    # print
    background = im.open(in_radar_image)
    foreground = im.open(overlayFile)
    shadow =  im.open(shadowFile).convert("RGBA")
    shadowTotransparent = shadow.getdata()
    shadowImage = []
    for item in shadowTotransparent :
        if item[0] == 0 and item[1] == 0 and item[2] == 0 and item[3] == 255:
            shadowImage.append((0, 0, 0, 100))  
        else:
            shadowImage.append(item)

    shadow.putdata(shadowImage)
    x, y = shadow.size
    background.paste(shadow, (0, 0, x, y), shadow)
    background.paste(foreground, (0, 0), foreground)
    background.save(outputFile)

    # print "overlay_radar_image.py ends ......"
    # print

if __name__ == '__main__':
    overlay_radar_image()
    
