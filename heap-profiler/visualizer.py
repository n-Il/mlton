#Matt Harris
#This code reads in the binary data and graphs it using matplotlib
import sys
import os
import matplotlib.pyplot as plt
from matplotlib.ticker import FormatStrFormatter
import numpy as np
from mplcursors import cursor
import matplotlib.dates
from datetime import datetime,timezone,timedelta


def read_in_data(location):
    objects = []
    datapoints = []
    counter = 1
    with open(location,"rb") as f:
        while (b := f.read(8)):
            datapoints.append(
            (
            counter,
            int.from_bytes(b,"little"),
            int.from_bytes(f.read(8),"little"),
            int.from_bytes(f.read(8),"little"),
            int.from_bytes(f.read(4),"little"),
            int.from_bytes(f.read(4),"little"),
            int.from_bytes(f.read(4),"little"),
            int.from_bytes(f.read(4),"little"),
            int.from_bytes(f.read(4),"little"),
            ))
            counter += 1
    return datapoints

def main():
    if len(sys.argv) < 2:
        print("give the file location that you output using @MLton heap-profiling <filename> --")
    else:
        datapoints = read_in_data(sys.argv[1])
        #for datapoint in datapoints:
        #    print(datapoint)
        #graphs
        graph_one(datapoints)
        graph_five(datapoints)
    return


#Past graphs require different data layout
def graph_one(data):
    #GRAPH 1 
    plt.figure("Live Data and Heapsize in Bytes per GC")
    gc_num_double_line = ([],[],[])
    for d in data:
        gc_num_double_line[0].append(d[0])#gc number 0 indexed
        gc_num_double_line[1].append(d[2])#live data
        gc_num_double_line[2].append(d[3])#heapsize data
    plt.xlabel("GC Number")
    plt.ylabel("Live Data and Heap Size in Bytes")
    plt.plot(gc_num_double_line[0],gc_num_double_line[1])
    plt.plot(gc_num_double_line[0],gc_num_double_line[2]) 
    #plt.xticks(gc_num_double_line[0])
    plt.gca().yaxis.set_major_formatter(FormatStrFormatter('%d'))
    #dots = plt.scatter(gc_num_double_line[0]+gc_num_double_line[0],gc_num_double_line[1]+gc_num_double_line[2],color='none')
    #cursor(dots,hover=True)
    plt.show()
    return

def graph_two(data):
    plt.figure("Live Data and Heapsize in Bytes per milliseconds passed")
    gc_ts_double_line = ([],[],[])
    for d in data:
        #gc_ts_double_line[0].append(datetime.fromtimestamp(d[1],timezone.utc) + timedelta(milliseconds=d[0]))#gc number timestamp
        gc_ts_double_line[0].append(d[1])#milliseconds
        gc_ts_double_line[1].append(d[2])#live data
        gc_ts_double_line[2].append(d[3])#heapsize data
    plt.xlabel("Milliseconds Passed")
    plt.ylabel("Live Data and Heap Size in Bytes")
    plt.plot(gc_ts_double_line[0],gc_ts_double_line[1])
    plt.plot(gc_ts_double_line[0],gc_ts_double_line[2]) 
    #plt.xticks(gc_ts_double_line[0])
    plt.gca().yaxis.set_major_formatter(FormatStrFormatter('%d'))
    dots = plt.scatter(gc_ts_double_line[0]+gc_ts_double_line[0],gc_ts_double_line[1]+gc_ts_double_line[2],color='none')
    cursor(dots,hover=True)
    #plt.gca().xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%H:%M:%S'))
    plt.show()
    return

def graph_three(data):
    plt.figure("Live Data and Heapsize in Bytes per milliseconds passed STACK")
    gc_ts_double_line = ([],[],[])
    for d in data:
        #gc_ts_double_line[0].append(datetime.fromtimestamp(d[1],timezone.utc) + timedelta(milliseconds=d[0]))#gc number timestamp
        gc_ts_double_line[0].append(d[1])#milliseconds
        gc_ts_double_line[1].append(d[2])#live data
        gc_ts_double_line[2].append(d[3])#heapsize data
    plt.xlabel("Milliseconds Passed")
    plt.ylabel("Live Data and Heap Size in Bytes")
    plt.stackplot(gc_ts_double_line[0],[gc_ts_double_line[1],gc_ts_double_line[2]])
    #plt.xticks(gc_ts_double_line[0])
    plt.gca().yaxis.set_major_formatter(FormatStrFormatter('%d'))
    dots = plt.scatter(gc_ts_double_line[0]+gc_ts_double_line[0],gc_ts_double_line[1]+gc_ts_double_line[2],color='none')
    cursor(dots,hover=True)
    #plt.gca().xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%H:%M:%S'))
    plt.show()
    return

#triple line chartz
#through this chart realized this is the same data
def graph_four(data):
    plt.figure("Live Data and Heapsize in Bytes per GC")
    fields_rotated = ([],[],[],[])
    for d in data:
        fields_rotated[0].append(d[0])#gc number 0 indexed
        fields_rotated[1].append(d[2])#live data
        fields_rotated[2].append(d[3])#heapsize data
        fields_rotated[3].append(d[4])#oldgensize data
    plt.xlabel("GC Number")
    plt.ylabel("Live Data, Heap Size, oldGenSize")
    plt.plot(fields_rotated[0],fields_rotated[1],color = "red")
    plt.plot(fields_rotated[0],fields_rotated[2],color = "yellow") 
    plt.plot(fields_rotated[0],fields_rotated[3],color = "blue") 
    #plt.xticks(gc_num_double_line[0])
    plt.gca().yaxis.set_major_formatter(FormatStrFormatter('%d'))
    dots = plt.scatter(fields_rotated[0]+fields_rotated[0]+fields_rotated[0],fields_rotated[1]+fields_rotated[2]+fields_rotated[3],color='none')
    cursor(dots,hover=True)
    plt.show()
    return


#graph 5
#this is going to take the 5 random moduluses and do an area plot 
def graph_five(data):
    plt.figure("utilization based on new header data mod 5")
    x_values = []
    y_values = ([],[],[],[],[])
    for d in data:
        x_values.append(d[1])
        y_values[0].append(d[4])
        y_values[1].append(d[5])
        y_values[2].append(d[6])
        y_values[3].append(d[7])
        y_values[4].append(d[8])
    plt.xlabel("Milliseconds Passed")
    plt.ylabel("new header mod 5")
    plt.stackplot(x_values,[y_values[0],y_values[1],y_values[2],y_values[3],y_values[4]])
    plt.gca().yaxis.set_major_formatter(FormatStrFormatter('%d'))
    #dots = plt.scatter(gc_ts_double_line[0]+gc_ts_double_line[0],gc_ts_double_line[1]+gc_ts_double_line[2],color='none')
    #cursor(dots,hover=True)
    plt.show()
    return


if __name__ == '__main__':
    main()
