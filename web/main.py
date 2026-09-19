import socket
import webbrowser
from PIL import Image
from pyzbar.pyzbar import decode

HOST = '0.0.0.0'
PORT = 8080
EXPECTED_BYTES = 256 * 192

def start_server():
    print(f"Starting DSi Proxy Server on port {PORT}...")
    
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((HOST, PORT))
        s.listen()
        
        print("Waiting for Ngrok tunnel connection...\n")

        while True:
            conn, addr = s.accept()
            with conn:
                print(f"Connection received. Downloading frame...")
                data = bytearray()
                
                while len(data) < EXPECTED_BYTES:
                    packet = conn.recv(EXPECTED_BYTES - len(data))
                    if not packet:
                        break
                    data.extend(packet)
                
                if len(data) == EXPECTED_BYTES:
                    process_frame(data)
                else:
                    print(f"Error: Incomplete frame ({len(data)} bytes).")

def process_frame(raw_bytes):
    # Reconstruct the grayscale image
    image = Image.frombuffer("L", (256, 192), bytes(raw_bytes), "raw", "L", 0, 1)
    image.save("dsi_capture.png") # Save locally to debug camera focus
    
    decoded_objects = decode(image)
    
    if not decoded_objects:
        print("-> No QR code detected. Check dsi_capture.png for focus/lighting.")
        return

    for obj in decoded_objects:
        qr_data = obj.data.decode('utf-8')
        print(f"-> QR Code Found: {qr_data}")
        
        if qr_data.startswith("http"):
            print("-> Opening in browser...\n")
            webbrowser.open(qr_data)
        else:
            print("-> Not a valid URL.\n")

if __name__ == "__main__":
    start_server()