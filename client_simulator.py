import socket, time

pacotes = ['11:00:00:00:11:11;00:00:00:00:00:00;-60;2','00:00:00:11:11:11;00:00:00:00:00:00;-30;2','11:11:22:AA:00:00;00:00:00:11:11:11;-2;3','00:11:22:00:11:22;11:00:00:00:11:11;-30;3','22:11:00:00:11:22;11:00:00:00:11:11;-60;3','AA:00:00:00:00:AA;22:11:00:00:11:22;-40;3']

for pck in range(0,len(pacotes)): 
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect(('192.168.0.6',8000))

	s.send(pacotes[pck].encode())
	s.close()
	time.sleep(2)

time.sleep(1)

atualiza = ['22:11:00:00:11:22;00:00:00:00:00:00;-80;2','00:11:22:00:11:22;00:00:00:11:11:11;-12;3','AA:00:00:00:00:AA;00:11:22:00:11:22;-30;4']

for atz in range(0,len(atualiza)):
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect(('192.168.0.6',8000))

	s.send(atualiza[atz].encode())
	s.close()
	time.sleep(2)	