### Matt Harris
### This code reads in the binary data and graphs it using matplotlib.
###
import sys
import os
import matplotlib.pyplot as plt
from matplotlib.ticker import FormatStrFormatter
import numpy as np
from mplcursors import cursor
import matplotlib.dates
from datetime import datetime,timezone,timedelta


### The binary data reading code is not perfectly portable, it is dependant on the system's C datatype sizes
### It is not impossible to solve this issue in the future.
BOOL_BYTE_SIZE = 1
INT_BYTE_SIZE = 4
UINT32_T_BYTE_SIZE = 4
SIZE_T_BYTE_SIZE = 8
UINTMAX_T_BYTE_SIZE = 8

#reads the profiling data from file to dictionary
def read_data(location, debug = False):
    data = dict()
    with open(location,"rb") as f:
        #bool location profiling
        location_profiling_bool = int.from_bytes(f.read(BOOL_BYTE_SIZE),"little") 
        #bool lifetime profiling
        lifetime_profiling_bool = int.from_bytes(f.read(BOOL_BYTE_SIZE),"little") 
        #int accuracy
        lifetime_accuracy_int = int.from_bytes(f.read(INT_BYTE_SIZE),"little") 
        #uint32_t sourcenameslength
        location_source_names_length_uint32_t = int.from_bytes(f.read(UINT32_T_BYTE_SIZE),"little") 
        data["location_profiling"] = True if location_profiling_bool == 1 else False
        data["lifetime_profiling"] = True if lifetime_profiling_bool == 1 else False
        data["lifetime_accuracy"] = lifetime_accuracy_int
        data["source_names_length"] = location_source_names_length_uint32_t
        if debug:
            print("location_profiling_bool:"+str(location_profiling_bool))
            print("lifetime_profiling_bool:"+str(lifetime_profiling_bool))
            print("lifetime_accuracy_int:"+str(lifetime_accuracy_int))
            print("location_source_names_length_uint32_t:"+str(location_source_names_length_uint32_t))
        #if location profiling is true then 
        if location_profiling_bool:
            source_names = []
            #for x from 0 to sourceNamesLength
            for x in range(location_source_names_length_uint32_t):
                #size_t len
                source_string_len_size_t = int.from_bytes(f.read(SIZE_T_BYTE_SIZE),"little")
                #bytes of string
                source_name_string = f.read(source_string_len_size_t).decode()
                source_names.append(source_name_string)
                if debug:
                    print("source_string_len_size_t:"+str(source_string_len_size_t))
                    print("source_name_string:"+source_name_string)
            data["source_names"] = source_names
        counter = 0
        garbage_collections = []
        while (b := f.read(UINTMAX_T_BYTE_SIZE)): 
            counter += 1
            gc = dict()
            gc["#"] = counter
            #uintmax_t time in ms
            time_in_ms_uintmax_t = int.from_bytes(b,"little")
            #size_t utilization
            heap_live_bytes_size_t = int.from_bytes(f.read(SIZE_T_BYTE_SIZE),"little")
            #size_t heap total size
            heap_total_size_size_t = int.from_bytes(f.read(SIZE_T_BYTE_SIZE),"little")
            #size_t object count
            heap_num_objects_size_t = int.from_bytes(f.read(SIZE_T_BYTE_SIZE),"little")
            gc["time_ms"] = time_in_ms_uintmax_t
            gc["live_data"] = heap_live_bytes_size_t
            gc["total_size"] = heap_total_size_size_t
            gc["num_objects"] = heap_num_objects_size_t
            if debug:
                print("time_in_ms_uintmax_t:"+str(time_in_ms_uintmax_t))
                print("heap_live_bytes_size_t:"+str(heap_live_bytes_size_t))
                print("heap_total_size_size_t:"+str(heap_total_size_size_t))
                print("heap_num_objects_size_t:"+str(heap_num_objects_size_t))
            #if gc survived
            if (lifetime_profiling_bool == 1):
                #lifetimes 1 2 3 4 5 <10 <100 <1000 <10000 <100000 <1000000 <10000000 and longer
                num_objects_per_lifetime = [0] * 13
                sum_size_objects_per_lifetime = [0] * 13
                #read 13 size_t #obj array
                for i in range(13):
                    num_objects_per_lifetime[i] = int.from_bytes(f.read(SIZE_T_BYTE_SIZE),"little") 
                #read 13 size_t sumsizeobj array
                for i in range(13):
                    sum_size_objects_per_lifetime[i] = int.from_bytes(f.read(SIZE_T_BYTE_SIZE),"little") 
                gc["objects_per_lifetime"] = num_objects_per_lifetime
                gc["bytes_per_lifetime"] = sum_size_objects_per_lifetime
                if debug:
                    print("num_objects_per_lifetime:"+str(num_objects_per_lifetime))
                    print("sum_size_objects_per_lifetime:"+str(sum_size_objects_per_lifetime))
            #if location
            if (location_profiling_bool == 1):
                #for x from 0 to sourceNamesLength
                num_objects_per_location = [0] * location_source_names_length_uint32_t
                sum_size_objects_per_location = [0] * location_source_names_length_uint32_t
                for i in range(location_source_names_length_uint32_t):    
                    # number objects with this source size_t
                    num_objects_per_location[i] = int.from_bytes(f.read(SIZE_T_BYTE_SIZE),"little") 
                    # sum size of objects with source  size_t
                    sum_size_objects_per_location[i] = int.from_bytes(f.read(SIZE_T_BYTE_SIZE),"little") 
                gc["objects_per_location"] = num_objects_per_location
                gc["bytes_per_location"] = sum_size_objects_per_location
                if debug:
                    print("num_objects_per_location:"+str(num_objects_per_location))
                    print("sum_size_objects_per_location:"+str(sum_size_objects_per_location))
            garbage_collections.append(gc) 
        data["garbage_collections"] = garbage_collections
        if debug:
            ba = bytearray(f.read())
            for b in ba:
                print("leftover byte")
    return data

#TODO: cut this out after working
#def read_in_data(location):
#    objects = []
#    datapoints = []
#    counter = 1
#    with open(location,"rb") as f:
#        while (b := f.read(8)):
#            datapoints.append(
#            (
#            counter,
#            int.from_bytes(b,"little"),
#            int.from_bytes(f.read(8),"little"),
#            int.from_bytes(f.read(8),"little"),
#            int.from_bytes(f.read(4),"little"),
#            int.from_bytes(f.read(4),"little"),
#            int.from_bytes(f.read(4),"little"),
#            int.from_bytes(f.read(4),"little"),
#            int.from_bytes(f.read(4),"little"),
#            ))
#            counter += 1
#    return datapoints

def main():
    if len(sys.argv) < 2:
        print("give the file location that you output using @MLton heap-profiling <filename> --")
    else:
        data = read_data(sys.argv[1])
        #normal graphs 
        #if loc then those graphs
        #if life then those graphs
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
