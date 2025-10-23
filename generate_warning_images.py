'''
Created on 2013.08.02

@author: Prog(A)42
'''

import os
import sys
import csv
import numpy as np

from math import radians, sqrt
from matplotlib.pyplot import figure

MS_TO_KNOTS = 1.94384449
WSA_IN_KNOTS = 15
MBA_IN_KNOTS = 30
WSA_IN_MS = WSA_IN_KNOTS / MS_TO_KNOTS
MBA_IN_MS = MBA_IN_KNOTS / MS_TO_KNOTS
WSA_MSG = "WSA"
MBA_MSG = "MBA"
WSA_TAG = 0
MBA_TAG = 1
ERR_TAG = 2

LAT_LON_OFFSET = 1000000.0
R = 6371

def compute_distance_bearing(lat1, lon1, lat2, lon2):
    dLat = lat2 - lat1
    dLon = lon2 - lon1
    a = np.sin(dLat/2) * np.sin(dLat/2) + np.sin(dLon/2) * np.sin(dLon/2) * np.cos(lat1) * np.cos(lat2)
    c = 2 * np.arctan2(np.sqrt(a), np.sqrt(1-a))
    d = R * c
    y = np.sin(dLon) * np.cos(lat2)
    x = np.cos(lat1) * np.sin(lat2) - np.sin(lat1) * np.cos(lat2) * np.cos(dLon)
    brng = np.arctan2(y, x)
    brng = (np.degrees(brng) + 180) % 360
    return d, brng

def find_xy(x1, y1, r, slp, sign):
    x = sign * sqrt(r**2 / (1+slp**2)) + x1
    y = sign * sqrt(r**2 / (1+slp**2)) * slp + y1
    return x, y

def rect_to_polar(x, y):
    hypotenuse = np.hypot(x,y)
    angle = np.arctan2(y,x)
    return hypotenuse, angle

def plot_shape_circle(cx, cy, r, ax, color, alpha):
    step = np.arange(0,360)
    x = cx + r * np.cos(step)
    y = cy + r * np.sin(step)
    hypot, angle = rect_to_polar(x,y)   
    ax.fill(angle, hypot, color, linewidth=0, alpha=alpha)    
    
def plot_warning_shape(data, ax, ylim):
    data = np.array(data.split(","))
    if data[-1] == WSA_MSG:
        data[-1] = WSA_TAG
    elif data[-1] == MBA_MSG:
        data[-1] = MBA_TAG
    else:
        data[-1] = ERR_TAG
    data = data.astype(np.float)
    
    """ 
        x1, y1, x2, y2, rad, loss = data
        By using the NorthPolarAxes, 
        the x-y coordinates are inversely.  
    """ 
    
    y1, x1, y2, x2, rad, loss, tag = data
    
    if tag == ERR_TAG:
        exit
    
    if x1-x2 == 0:
        slp1 = 1
        slp2 = 0
    else:
        slp1 = (y1-y2)/(x1-x2)
        slp2 = -1/slp1

    if loss >= MBA_IN_KNOTS:
        color = "r"
	textcolor = "k"
    else:
        color = "b"
	textcolor = "w"
    alpha = 1
 
    cx = (x1+x2)/2
    cy = (y1+y2)/2
    hypot, angle = rect_to_polar(cx, cy)
    if int(hypot) <= int(ylim):
        ax.text(angle, hypot, int(loss), fontsize=8, horizontalalignment='center', verticalalignment='center', color=textcolor)
   
    x3, y3 = find_xy(x1, y1, rad, slp2, 1)
    x4, y4 = find_xy(x1, y1, rad, slp2, -1)
    x5, y5 = find_xy(x2, y2, rad, slp2, 1)
    x6, y6 = find_xy(x2, y2, rad, slp2, -1)
        
    slptest = (y4-y5)/(x4-x5)
    x = []
    y = []
    if np.allclose(slp1, slptest):
        x = np.array([x3,x6,x5,x4])
        y = np.array([y3,y6,y5,y4])
    else:
        x = np.array([x3,x5,x6,x4])
        y = np.array([y3,y5,y6,y4])
    r, a = rect_to_polar(x,y)
    
    ax.fill(a, r, color, linewidth=0, alpha=alpha)
    plot_shape_circle(x1, y1, rad, ax, color, alpha)
    plot_shape_circle(x2, y2, rad, ax, color, alpha)
 
def generate_warning_images():
    print "generate_warning_images starts ......"
    print 
    if len(sys.argv) != 3:
        print "Not enough arguments"
        sys.exit(1)
    
    in_warning = sys.argv[1]
    input_radar = sys.argv[2]
        
    print "Warning msg: " + in_warning
    print "Input Radar: " + input_radar
    print

    radar_lat = 0
    radar_lon = 0
    axes_left = 0
    axes_bottom = 0
    axes_width = 0
    axes_height = 0
    ytick_pos_x = 0
    ytick_pos_y = 0

    with open('./config/warning_images.conf') as csvfile:
        reader = csv.reader(csvfile, delimiter=',')
        for row in reader:
            if row[0] == input_radar:
                row = np.array(row[1:])
                row = row.astype(np.float)
                radar_lat, radar_lon, axes_left, axes_bottom, axes_width, axes_height, ytick_pos_x, ytick_pos_y = row

    radar_lat = radians(float(radar_lat))
    radar_lon = radians(float(radar_lon))
    
    overlayFile = os.path.basename(in_warning)
    filename = os.path.splitext(overlayFile)[0]
    timestamp = filename.split("_")[0]
    timestamp = timestamp.replace("T", "")

    output_path = os.path.dirname(in_warning)
    output_path = output_path.replace("/output", "")
    overlayFile = output_path + "/overlays/" + filename + ".png"
            
    print "Generate warning message overlay ......"
    print "Overlay: " + overlayFile
    print

    fig = figure(figsize=(5,5))

    ax = fig.add_axes([axes_left, axes_bottom, axes_width, axes_height], polar=True, frame_on=False)
    ax.set_theta_zero_location("N")
    ax.set_theta_direction(-1)
    
    ax.grid(False)  
    ax.set_xticks(np.arange(0, 2*np.pi, 10 * np.pi / 180))
    ax.set_xticklabels(np.arange(0, 360, 10), visible=False)
    
    x_pos = 0.60	#0.6
    y_pos = 0.51    #0.45
    pos_step = 0.033   
    ylim = 20    

    ax.set_ylim(ylim)
    ax.set_yticks(np.arange(2, ylim, 2))
    ax.set_yticklabels(range(2, ylim, 2), position=[ytick_pos_x, ytick_pos_y], visible=False) 
          
    try:
        index = 0
        x_pos = x_pos
        y_pos = y_pos - 3 * pos_step
        rw_ypos = y_pos
        with open(in_warning) as infile:    
            content = [line.strip() for line in infile]
            for line in content:
                if index > 0 and index < 13:                      
                    rw_xpos = x_pos 
                    rw_ypos = rw_ypos - pos_step
                    fig.text(rw_xpos, rw_ypos, line, weight="bold", fontsize =10.5) 
                elif index >= 13:
                    plot_warning_shape(line, ax, ylim)
                index = index + 1
    except IOError:
        print "Error: Alt Files doesnt exist."
    fig.savefig(overlayFile, transparent=True)

    print "generate_warning_images ends ......"
    print 

if __name__ == '__main__':
    generate_warning_images()
    
