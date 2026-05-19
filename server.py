import socket
import csv
from datetime import datetime

UDP_IP = "0.0.0.0"
UDP_PORT = 4210
CSV_FILE = "bft5.csv"

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

with open(CSV_FILE, "a+", newline="") as f:
    writer = csv.writer(f)
    if f.tell() == 0:
        writer.writerow(["timestamp", "node", "ax", "ay", "az", "gx", "gy", "gz", "t_ms"])
    
    print(f"Listening on port {UDP_PORT}...")
    while True:
        data, addr = sock.recvfrom(256)
        message = data.decode().strip()
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        parts = message.split(",")
        if len(parts) == 7:
            node, ax, ay, az, gx, gy, gz = parts
            writer.writerow([timestamp, node, ax, ay, az, gx, gy, gz, ""])
        elif len(parts) == 8:
            node, ax, ay, az, gx, gy, gz, t_ms = parts
            writer.writerow([timestamp, node, ax, ay, az, gx, gy, gz, t_ms])
        
        f.flush()
        print(f"[{timestamp}] {message}")