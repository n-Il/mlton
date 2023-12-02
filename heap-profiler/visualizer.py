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

# Data dictionary
# "location_profiling" BOOL
# "lifetime_profiling" BOOL
# "lifetime_accuracy" INT
# if location_profiling" 
#   "source_names_length" INT
#   "source_names" LIST of STRING
# garbage_collections LIST OF {
#   "#" INT
#   "time_ms" INT
#   "live_data" INT
#   "total_size" INT
#   "num_objects" INT
#   if "lifetime_profiling"
#       "objects_per_lifetime" LIST of INT
#       "bytes_per_lifetime" LIST of INT
#   if location_profiling 
#       "objects_per_location" LIST of INT
#       "bytes_per_location" LIST of INT
# }

#in the case of lifetimes we are doing are split by these values
#lifetimes 1 2 3 4 5 <10 <100 <1000 <10000 <100000 <1000000 <10000000 and longer

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

def main():
    if len(sys.argv) < 2:
        print("give the file location that you output using @MLton heap-profiling <filename> --")
    else:
        data = read_data(sys.argv[1])
        #graphs regardless of options
        number_objects_per_gc_graph(data)
        number_objects_per_ms_graph(data)
        live_data_and_heap_size_per_gc_graph(data)
        live_data_and_heap_size_per_ms_graph(data)
        #if loc then those graphs
            #top 10 and rest bundled count per location per gc
            #top 10 and rest bundled count per location per ms
            #top 10 and rest bundled sum_size per location per gc
            #top 10 and rest bundled sum_size per location per ms
        if data["lifetime_profiling"]: 
            sum_objects_size_per_lifetime_per_gc_graph(data)
            sum_objects_size_per_lifetime_per_ms_graph(data)
            count_objects_per_lifetime_per_gc_graph(data) 
            count_objects_per_lifetime_per_ms_graph(data)

    return

def number_objects_per_gc_graph(data):
    #GRAPH 1 
    plt.figure("Live Objects per Garbage Collection")
    x = []#gc #
    y = []#gc num_objects
    for gc in data["garbage_collections"]:
        x.append(gc["#"])
        y.append(gc["num_objects"])
    plt.xlabel("GC Number")
    plt.ylabel("Number of Live Objects")
    plt.plot(x,y)
    #the following code can be used to add on-hover effects to the plot
    #dots = plt.scatter(x,y,color='none')
    #cursor(dots,hover=True)
    plt.show()
    return

def number_objects_per_ms_graph(data):
    #GRAPH 1 
    plt.figure("Live Objects by Milliseconds Passed")
    x = []#gc time_ms
    y = []#gc num_objects
    for gc in data["garbage_collections"]:
        x.append(gc["time_ms"])
        y.append(gc["num_objects"])
    plt.xlabel("Elapsed Milliseconds of Execution ")
    plt.ylabel("Number of Live Objects")
    plt.plot(x,y)
    plt.show()
    return

def live_data_and_heap_size_per_gc_graph(data):
    plt.figure("Heap Size and Live Data in Bytes per Garbage Collection")
    x12 = []#gc #
    y1 = []#gc total_size
    y2 = []#gc live_data
    for gc in data["garbage_collections"]:
        x12.append(gc["#"])
        y1.append(gc["total_size"]-gc["live_data"])
        y2.append(gc["live_data"])
    plt.xlabel("GC Number")
    plt.ylabel("Heap Size and Live Data in Bytes")
    plt.stackplot(x12,[y2,y1])
    #plt.xticks(gc_ts_double_line[0])
    #plt.gca().yaxis.set_major_formatter(FormatStrFormatter('%d'))
    #dots = plt.scatter(gc_ts_double_line[0]+gc_ts_double_line[0],gc_ts_double_line[1]+gc_ts_double_line[2],color='none')
    #cursor(dots,hover=True)
    #plt.gca().xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%H:%M:%S'))
    plt.show()
    return

def live_data_and_heap_size_per_ms_graph(data):
    plt.figure("Heap Size and Live Data in Bytes by Milliseconds Passed")
    x12 = []#gc time_ms
    y1 = []#gc total_size
    y2 = []#gc live_data
    for gc in data["garbage_collections"]:
        x12.append(gc["time_ms"])
        y1.append(gc["total_size"]-gc["live_data"])
        y2.append(gc["live_data"])
    plt.xlabel("Elapsed Milliseconds of Execution ")
    plt.ylabel("Heap Size and Live Data in Bytes")
    plt.stackplot(x12,[y2,y1])
    #plt.xticks(gc_ts_double_line[0])
    #plt.gca().yaxis.set_major_formatter(FormatStrFormatter('%d'))
    #dots = plt.scatter(gc_ts_double_line[0]+gc_ts_double_line[0],gc_ts_double_line[1]+gc_ts_double_line[2],color='none')
    #cursor(dots,hover=True)
    #plt.gca().xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%H:%M:%S'))
    plt.show()
    return

def sum_objects_size_per_lifetime_per_gc_graph(data):
    plt.figure("Data Utilization of Lifetime Magnitudes per GC(until I figure out legend it's low survived at bottom)")
    x = []#gc #
    accuracy = data["lifetime_accuracy"]
    lifetimes = [1*accuracy,2*accuracy,3*accuracy,4*accuracy,5*accuracy,10*accuracy,100*accuracy,1000*accuracy,10000*accuracy,100000*accuracy,1000000*accuracy,10000000*accuracy]#TODO replace with map
    y = ([],[],[],[],[],[],[],[],[],[],[],[],[])
    for gc in data["garbage_collections"]:
        x.append(gc["#"])
        for i in range(13):
            y[i].append(gc["bytes_per_lifetime"][i])
    plt.xlabel("GC Number")
    ylabelstr = ""
    counter = 0
    for val in lifetimes:
        if counter > 4:
            ylabelstr+="<"
        counter+=1
        ylabelstr+=str(val)
        ylabelstr+=","
    ylabelstr += "or greater GC-survived"
    plt.ylabel(ylabelstr)
    plt.stackplot(x,[y[0],y[1],y[2],y[3],y[4],y[5],y[6],y[7],y[8],y[9],y[10],y[11],y[12]])
    plt.show()
    return

def sum_objects_size_per_lifetime_per_ms_graph(data):
    plt.figure("Data utilization of Lifetime Magnitudes by Elapsed Milliseconds(until I figure out legend it's low survived at bottom)")
    x = []#gc time_ms
    accuracy = data["lifetime_accuracy"]
    lifetimes = [1*accuracy,2*accuracy,3*accuracy,4*accuracy,5*accuracy,10*accuracy,100*accuracy,1000*accuracy,10000*accuracy,100000*accuracy,1000000*accuracy,10000000*accuracy]#TODO replace with map
    y = ([],[],[],[],[],[],[],[],[],[],[],[],[])
    for gc in data["garbage_collections"]:
        x.append(gc["time_ms"])
        for i in range(13):
            y[i].append(gc["bytes_per_lifetime"][i])
    plt.xlabel("Elapsed Milliseconds of Execution ")
    ylabelstr = ""
    counter = 0
    for val in lifetimes:
        if counter > 4:
            ylabelstr+="<"
        counter+=1
        ylabelstr+=str(val)
        ylabelstr+=","
    ylabelstr += "or greater GC-survived"
    plt.ylabel(ylabelstr)
    plt.stackplot(x,[y[0],y[1],y[2],y[3],y[4],y[5],y[6],y[7],y[8],y[9],y[10],y[11],y[12]])
    plt.show()
    return

def count_objects_per_lifetime_per_gc_graph(data):
    plt.figure("Object Count of Lifetime Magnitudes per GC(legend needs adding)")
    x = []#gc #
    accuracy = data["lifetime_accuracy"]
    lifetimes = [1*accuracy,2*accuracy,3*accuracy,4*accuracy,5*accuracy,10*accuracy,100*accuracy,1000*accuracy,10000*accuracy,100000*accuracy,1000000*accuracy,10000000*accuracy]#TODO replace with map
    y = ([],[],[],[],[],[],[],[],[],[],[],[],[])
    for gc in data["garbage_collections"]:
        x.append(gc["#"])
        for i in range(13):
            y[i].append(gc["objects_per_lifetime"][i])
    plt.xlabel("GC Number")
    ylabelstr = ""
    counter = 0
    for val in lifetimes:
        if counter > 4:
            ylabelstr+="<"
        counter+=1
        ylabelstr+=str(val)
        ylabelstr+=","
    ylabelstr += "or greater GC-survived"
    plt.ylabel(ylabelstr)
    for i in range(13):
        plt.plot(x,y[i])
    plt.show()
    return

def count_objects_per_lifetime_per_ms_graph(data):
    plt.figure("Object Count of Lifetime Magnitudes per GC(legend needs adding)")
    x = []#gc time_ms
    accuracy = data["lifetime_accuracy"]
    lifetimes = [1*accuracy,2*accuracy,3*accuracy,4*accuracy,5*accuracy,10*accuracy,100*accuracy,1000*accuracy,10000*accuracy,100000*accuracy,1000000*accuracy,10000000*accuracy]#TODO replace with map
    y = ([],[],[],[],[],[],[],[],[],[],[],[],[])
    for gc in data["garbage_collections"]:
        x.append(gc["time_ms"])
        for i in range(13):
            y[i].append(gc["objects_per_lifetime"][i])
    plt.xlabel("Elapsed Milliseconds of Execution ")
    ylabelstr = ""
    counter = 0
    for val in lifetimes:
        if counter > 4:
            ylabelstr+="<"
        counter+=1
        ylabelstr+=str(val)
        ylabelstr+=","
    ylabelstr += "or greater GC-survived"
    plt.ylabel(ylabelstr)
    for i in range(13):
        plt.plot(x,y[i])
    plt.show()
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
