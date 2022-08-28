# Python program to implement server side  of the chat application
import socket
import select
import sys
import threading
 


server= socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
IP_address="127.0.0.1"
server.bind((IP_address, 5040))



dclient = {}
def clientthread(conn, addr):
	

	# sends a message to the client whose user object is conn
	
	reg=False
	while (not reg):
		message = conn.recv(1024).decode()
		if message:
			
			a,b,c = message.split()
			if (a=="REGISTER"  and b=="TOSEND"):
				if c.isalnum():
					senderusername = c
					sendmsg="REGISTERED TOSEND "+c+"\n"
					s=sendmsg.encode()
					conn.send(s)
					print(c+" connected to send")
					reg=True
				else:
					sendmsg = "ERROR 100 Malformed username\n \n"
					s=sendmsg.encode()
					conn.send(s)
			
			elif (a=="REGISTER" and b=="TORECV"):
				if c.isalnum():
						
					sendmsg = "REGISTERED TORECV "+c+"\n"
					sendmsg=sendmsg.encode()
					conn.send(sendmsg)
					dclient[c]=conn
					senderusername = c
					print(c+" connected to recieve")
					reg=True
				else:
					conn.send("ERROR 100 Malformed username\n \n".encode())
			else:
				conn.send("ERROR 101 No user registered \n \n".encode())

		else:
			remove(conn,dclient)

	while True:
			try:
				message = conn.recv(1024).decode()
				
				if message:
					array =message.split()
					a=array[0]+" "+array[1]
					b = array[3]
					strtojoin=" "
					c= strtojoin.join(array[4:])

					
					if  int(b)==len(c) :
						bh ,username=a.split()
						if (bh =="SEND" and username=="ALL"):
							msg="FORWARD "+ senderusername+" \n Content-length: "+b+ "\n\n"+c
							
							sendall(msg, conn,senderusername)

						elif (bh=="SEND") :
							
							msg="FORWARD "+ senderusername+" \n Content-length: "+b+ "\n\n"+c
							
							try:
								
								dclient[username].send(msg.encode())
								
								msg_from_recipient=dclient[username].recv(1024).decode()
							except:
								sendmsg = "ERROR 102 Unable to send\n\n"
								s=sendmsg.encode()
								conn.send(s)
							if msg_from_recipient:
								msg1,msg2 = msg_from_recipient.split()
								
								
								if (msg1=="RECEIVED" and msg2 == senderusername ):
									sendmsg="SENT "+ username+"\n \n"
									s=sendmsg.encode()
									conn.send(s)
									

								else:
									conn.send(msg_from_recipient) 
						

						else:
							conn.send("ERROR 103 Header incomplete \n \n".encode())
							remove(conn,dclient)
					else:
						conn.send("ERROR 103 Header incomplete \n \n".encode())
						remove(conn,dclient)
				
				else:
					remove(conn,dclient)

			except:
				remove(conn,dclient)



def sendall(message, conn,senderusername):
	m=message.encode()
	for key,clients in dclient.items():
		if clients!=conn:
			try:
				clients.send(m)
				msg_from_recipient=clients.recv(1024).decode()
			except:
				print("ERROR 102 Unable to send\n\n")

			if msg_from_recipient:
				msg1,msg2 = msg_from_recipient.split()
								
								
				if (msg1=="RECEIVED" and msg2 == senderusername ):
					sendmsg="SENT "+ key+"\n \n"
					s=sendmsg.encode()
					conn.send(s)
									

				else:
					conn.send(msg_from_recipient) 
				
			


def remove(conn,dclient):
	dclient = {key:val for key, val in dclient.items() if val != conn}
	for key,val in dclient.items():
		if val== conn:
			username = key
			print(username+" disconnected from server")


print("SERVER STARTED")
while True:

	"""Accepts a connection request and stores two parameters,
	conn which is a socket object for that user, and addr
	which contains the IP address of the client that just
	connected"""
	server.listen(1)
	conn, addr = server.accept()


	# creates and individual thread for every user
	# that connects

	threading.Thread(target=clientthread,args=(conn,addr,)).start()
		

conn.close()
server.close()
