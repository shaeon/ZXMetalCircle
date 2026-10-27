import socket, sys, time
s=socket.socket(); s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1); s.bind(('127.0.0.1',2000)); s.listen(1); s.settimeout(60)
c,a=s.accept(); print('conexión de', a, flush=True)
c.sendall(b'HOLA DESDE TCP\r\n')
c.settimeout(20)
try:
    d=c.recv(100); print('recibido:', d, flush=True)
except Exception as e: print('sin datos', e, flush=True)
time.sleep(1); c.close()
