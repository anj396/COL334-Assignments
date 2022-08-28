import socket
import struct;

import time;
import select;
import math;
import sys;
import csv;
import matplotlib.pyplot as plt
import numpy as np
import os


protocol = socket.getprotobyname("icmp");

#Checksum (16 bits), the 16-bit one's complement of the one's complement sum of the packet. 
#For IPv4, this is calculated from the ICMP message starting with the Type field

def pingchksum(str):
    sum = 0
    l=(int) (len(str) / 2) * 2
    for i in range(0,l,2):
        sum+= (str[i + 1])*256 + (str[i])
        sum &= 0xffffffff # handles overflow 
        
    if l< len(str):
        sum +=str[-1]
        sum &= 0xffffffff # overflow
    sum = (sum >> 16) + (sum & 0xffff)
    sum += (sum >> 16)
    answer = ~sum
    answer &=  0xffff
   
    answer = answer >> 8 | (answer << 8 & 0xff00)
    return answer

    

   

#`b' signed char --- integer
#`h' short --- integer
#B - Unsigned Char (8 bits)
#H - Unsigned Short (16 bits)

def newpacket(id):
    head = struct.pack('BBHHH', 8, 0, 0, id, 1)
    data = ''
    val= pingchksum(head + data.encode('utf-8'))
    head = struct.pack('BBHHH', 8, 0,socket.htons(val), id, 1)
    return head + data.encode('utf-8')
    




def gotping(sock,senttime):
    t=timeout
    i=1

    while (i>0):
        start_time = time.time()

        iodone= select.select([sock], [], [],t)
        if iodone[0] == []: 
            return 0
        recv_t = time.time()
        p, addr = sock.recvfrom(1024)
        
        header = p[-8:]
        
        type, code, sum, p_id, seq = struct.unpack('BBHHH', header)


        if p_id == 10:
            final = (recv_t - senttime) * 1000
            final=round(final,3)
            return (addr[0], final)
        t -= recv_t - senttime
        if t<= 0:
            return 0



def oneping(hostname,ttl):

    sock =socket.socket(socket.AF_INET, socket.SOCK_RAW, protocol)
    sock.setsockopt(socket.SOL_IP, socket.IP_TTL, ttl)
    p = newpacket(10)   #creating a paclet with random id
    
    
    while p:
        
        p = p[sock.sendto(p, (host, 1)):]

    ping_res = gotping(sock, time.time())
    sock.close()
    return ping_res

def threepings(hostname,ttl):


    one =oneping(hostname,ttl)
    two = oneping(hostname,ttl)
    three = oneping(hostname,ttl)
    tcsv=0

    if one == 0:
        s1 = '*'
    else:
        s1 = one[0] + "  " + str(one[1]) + ' ms'
        tcsv+=one[1]
        
    if two == 0:
        s2 = '*'
    else:
        s2 = two[0] + '  ' + str(two[1]) + ' ms'
        tcsv+=two[1]
    if three == 0:
        s3 = '*'
    else:
        s3 = three[0] + '  ' + str(three[1]) + ' ms'
        tcsv+=three[1]

    ans= s1 + ', ' + s2 + ', ' + s3

    if one == 0:
        return (ans,False,tcsv)
    return (ans,one[0] == host,tcsv)




#main

if len(sys.argv) == 1:
    print('Please enter a hostname.')
    sys.exit(1)

hostname =sys.argv[1]
host = socket.gethostbyname(hostname)

max_hops=30
timeout =3
if (len(sys.argv)==3):
    max_hops = int(sys.argv[2])


print("traceroute to "+hostname+" ("+host+"), "+str(max_hops)+" hops max")

try:
    x = []
    y = []

    for i in range(1, max_hops+1):
        (a, b,c) = threepings(host, i)

        x.append(i)
        y.append(c/3)
        print(str(i)+" "+a)
        if b: 
          break

    plt.plot(x, y, color = 'g', linestyle = 'dashed',
         marker = 'o',label = "RTT")
  
    plt.xticks(rotation = 25)
    plt.xlabel('No of Hops')
    plt.ylabel('RTT')
    plt.title('RTT vs Hops', fontsize = 20)
    plt.grid()
    plt.legend()
    plt.show()
    plt.savefig(hostname+'.png', bbox_inches='tight')
except Exception as e:
    print(e)










