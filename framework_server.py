import socket,time,random
from matplotlib import pyplot as plt

class received_message:
	position = 0.0
	text = ''
	def __init__(self,esp_mac,parent_mac,rssi,layer):
		self.esp_mac = esp_mac
		self.parent_mac = parent_mac
		self.rssi = int(rssi)
		self.layer = int(layer)

current_transmission = {}

plt.ion() #MODO INTERATIVO

fig = plt.figure()
grafico = fig.add_subplot(111,projection='polar')
grafico.set_yticklabels([])
grafico.set_xticklabels([])

while True:
	with socket.socket(socket.AF_INET,socket.SOCK_STREAM) as s:
		s.bind(("192.168.0.6",8000))
		s.listen()
		conn,addr = s.accept()
		with conn:
			print('Connected by',addr)
			while True:
				data = conn.recv(1024)
				dados = data.decode()
				time.sleep(0.5)
				if not data:
					break
				dados = dados.split(';')
				current_transmission[dados[0]] = received_message(dados[0],dados[1],dados[2],dados[3])
				current_transmission[dados[0]].position = (random.randint(0,360)*3.1415/180) 
				print("esp transmissor: {}\nesp pai: {}\nrssi: {}\ncamada: {}\n".format(current_transmission[dados[0]].esp_mac,current_transmission[dados[0]].parent_mac,current_transmission[dados[0]].rssi,current_transmission[dados[0]].layer))
				print(len(current_transmission))
				grafico.clear()
				grafico.set_yticklabels([])
				grafico.set_xticklabels([])
				for peer in current_transmission.values():
					grafico.scatter(peer.position,peer.layer,color='r')
					if peer.parent_mac in current_transmission.keys(): 
						plt.plot([peer.position,current_transmission[peer.parent_mac].position],[peer.layer,current_transmission[peer.parent_mac].layer],color='c')
					else:
						plt.plot([peer.position,0],[peer.layer,0],color='k')
				fig.canvas.draw_idle()#REDESENHA O GRAFICO
				plt.pause(0.1)
				conn.send('/n'.encode())
					