import socket
import select
import sys
 
from threading import *
 

try:
    sendsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
except socket.error as err:
    print ("socket creation failed with error %s" %(err))


try:
    recvsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
except socket.error as err:
    print ("socket creation failed with error %s" %(err))
 

sendsock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
recvsock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
IP_address="127.0.0.1"


 
recvsock.connect((IP_address, 5040))
sendsock.connect((IP_address, 5040))


def recvreg(sock,username):
    reg=False
    while (not reg):
        sendmsg="REGISTER TORECV "+ username+"\n \n"
        s=sendmsg.encode()
        sock.send(s)
        msg = sock.recv(1024).decode()
       
        if (msg =="REGISTERED TORECV "+username+"\n"):
            reg=True
            print("registered to recieve")
        elif (msg=="ERROR 100 Malformed username\n \n"):
            sock.close()

        

    

def sendreg(sock,username):
    reg=False
    while (not reg):
        sendmsg ="REGISTER TOSEND "+ username+"\n \n"
        s=sendmsg.encode()
        sock.send(s)
        msg = sock.recv(1024).decode()
        
        
        if (msg=="REGISTERED TOSEND "+username+"\n"):
            reg=True
            print("registered to send")
        elif (msg=="ERROR 100 Malformed username\n \n"):
            sock.close()



def recvthread(recvsock):
    while True:
        msg = recvsock.recv(1024).decode()
        
        array=msg.split()
        m1=array[0]
        m2=array[1]
        b=array[3]
        strtojoin=" "
        c= strtojoin.join(array[4:])
        
        if (m1 =="FORWARD" and int(b)==len(c)):
            
            
            sendmsg="RECEIVED "+ m2 +"\n \n"
            s=sendmsg.encode()
            recvsock.send(s)
            print(m2+" : "+c)
            
        else:
            recvsock.send("ERROR 103 Header Incomplete\n\n".encode())






def sendthread(sendsock):
    print("message format is : @username [message]")
    print("CHAT START")
    while True:
        message=input()
        print(username+" : "+message)
        a=message.split()
        recipient =a[0][1:]

        sendmsg="SEND "+ recipient+"\n Content-length: "+ str(len(message)-len(a[0])-1) +"\n\n" +message[len(a[0]):]
        s=sendmsg.encode()
        sendsock.send(s)
       

        recvdmsg= sendsock.recv(1024).decode()
       
        if (recvdmsg == "SENT "+ recipient+"\n\n"):
            print(username+" : "+message[len(a[0]):])
        else:
            print(recvdmsg)










username = input("Please Enter your username :")
recvreg(recvsock,username)
sendreg(sendsock,username)


Thread(target=recvthread,args=(recvsock,)).start()

Thread(target=sendthread,args=(sendsock,)).start()

