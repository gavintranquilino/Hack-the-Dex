import socket

# --- CHANGE THESE TO MATCH YOUR NGROK URL ---
TARGET_HOST = "2.tcp.ngrok.io"
TARGET_PORT = 28248  # Replace with your actual ngrok port

# The DSi sends exactly 256 x 192 = 49,152 bytes.
# This creates a dummy array of 49,152 zeros (a solid black image).
EXPECTED_BYTES = 256 * 192
dummy_payload = bytearray(EXPECTED_BYTES)

def test_connection():
    print(f"Connecting to {TARGET_HOST}:{TARGET_PORT}...")
    
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.connect((TARGET_HOST, TARGET_PORT))
            print("Connected! Sending 49KB dummy payload...")
            
            # sendall guarantees all 49,152 bytes are transmitted
            s.sendall(dummy_payload)
            print("Data sent successfully. Check your server terminal!")
            
        except Exception as e:
            print(f"Connection failed: {e}")

if __name__ == "__main__":
    test_connection()