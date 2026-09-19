import json
import os
import queue
import signal
import socket
import subprocess
import threading
import time
import tkinter as tk
import urllib.request
import webbrowser

from PIL import Image, ImageTk
from pyzbar.pyzbar import decode

SERVER_HOST = "0.0.0.0"
SERVER_PORT = 8080
EXPECTED_BYTES = 256 * 192
NGROK_API_URL = "http://127.0.0.1:4040/api/tunnels"


def create_server_socket(log):
    log(f"Starting DSi Proxy Server on port {SERVER_PORT}...")

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.settimeout(1.0)
    server_socket.bind((SERVER_HOST, SERVER_PORT))
    server_socket.listen()
    return server_socket


def start_server(server_socket, stop_event, log, show_image):
    with server_socket:
        log("Waiting for Ngrok tunnel connection...")

        while not stop_event.is_set():
            try:
                conn, addr = server_socket.accept()
            except socket.timeout:
                continue
            except OSError:
                if stop_event.is_set():
                    break
                raise

            with conn:
                conn.settimeout(1.0)
                log(f"Connection received from {addr}. Downloading frame...")
                data = bytearray()

                while len(data) < EXPECTED_BYTES and not stop_event.is_set():
                    try:
                        packet = conn.recv(EXPECTED_BYTES - len(data))
                    except socket.timeout:
                        continue
                    if not packet:
                        break
                    data.extend(packet)

                if len(data) == EXPECTED_BYTES and not stop_event.is_set():
                    process_frame(data, log, show_image)
                else:
                    log(f"Error: Incomplete frame ({len(data)} bytes).")


def process_frame(raw_bytes, log, show_image):
    image = Image.frombuffer("L", (256, 192), bytes(raw_bytes), "raw", "L", 0, 1)
    image.save("dsi_capture.png")
    show_image(image.copy())

    decoded_objects = decode(image)

    if not decoded_objects:
        log("-> No QR code detected. Check dsi_capture.png for focus/lighting.")
        return

    for obj in decoded_objects:
        qr_data = obj.data.decode("utf-8")
        log(f"-> QR Code Found: {qr_data}")

        if qr_data.startswith("http"):
            log("-> Opening in browser...")
            webbrowser.open(qr_data)
        else:
            log("-> Not a valid URL.")


def get_tcp_tunnel():
    with urllib.request.urlopen(NGROK_API_URL, timeout=1) as response:
        tunnels = json.load(response).get("tunnels", [])

    for tunnel in tunnels:
        public_url = tunnel.get("public_url", "")
        if public_url.startswith("tcp://"):
            address = public_url.removeprefix("tcp://")
            host, port = address.rsplit(":", 1)
            return host, port

    return None


def stop_ngrok(ngrok_process):
    if ngrok_process is None or ngrok_process.poll() is not None:
        return

    try:
        os.killpg(os.getpgid(ngrok_process.pid), signal.SIGTERM)
        ngrok_process.wait(timeout=3)
    except (ProcessLookupError, subprocess.TimeoutExpired):
        if ngrok_process.poll() is None:
            os.killpg(os.getpgid(ngrok_process.pid), signal.SIGKILL)
            ngrok_process.wait()


def run_app():
    log_queue = queue.Queue()
    stop_event = threading.Event()
    ngrok_process = None
    server_thread = None
    server_socket = None
    closing = False

    root = tk.Tk()
    root.title("DSi QR Scanner Connection")
    root.geometry("650x500")
    root.minsize(650, 500)
    root.resizable(False, False)

    def log(message):
        print(message, flush=True)
        log_queue.put(message)

    status_var = tk.StringVar(value="Starting ngrok TCP tunnel...")
    host_var = tk.StringVar(value="Host number: waiting...")
    port_var = tk.StringVar(value="Port: waiting...")

    header_frame = tk.Frame(root)
    header_frame.pack(fill="x", padx=32, pady=(22, 12))
    info_frame = tk.Frame(header_frame)
    info_frame.grid(row=0, column=0, sticky="nw", padx=(0, 24))
    preview_frame = tk.Frame(header_frame, width=256, height=192, bd=1, relief="solid")
    preview_frame.grid(row=0, column=1, sticky="ne")
    preview_frame.grid_propagate(False)
    header_frame.grid_columnconfigure(0, weight=1)

    tk.Label(info_frame, text="CONNECTION INPUTS", font=("TkDefaultFont", 14, "bold"), justify="left").pack(anchor="w", pady=(0, 12))
    tk.Label(info_frame, textvariable=host_var, anchor="w", width=28, font=("TkDefaultFont", 12)).pack(anchor="w", pady=2)
    tk.Label(info_frame, textvariable=port_var, anchor="w", width=28, font=("TkDefaultFont", 12)).pack(anchor="w", pady=2)
    tk.Label(info_frame, textvariable=status_var, anchor="w", width=28, wraplength=250, justify="left").pack(anchor="w", pady=(10, 0))

    preview_label = tk.Label(preview_frame, text="No frame yet", bg="black", fg="white")
    preview_label.pack(fill="both", expand=True)

    def show_image(image):
        try:
            root.after(0, update_preview, image)
        except tk.TclError:
            pass

    def update_preview(image):
        if closing:
            return
        preview_image = ImageTk.PhotoImage(image=image)
        preview_label.configure(image=preview_image, text="")
        preview_label.image = preview_image

    log_frame = tk.Frame(root)
    log_frame.pack(fill="both", expand=True, padx=32, pady=(0, 24))
    log_output = tk.Text(log_frame, height=15, width=72, state="disabled", wrap="word")
    log_scrollbar = tk.Scrollbar(log_frame, command=log_output.yview)
    log_output.configure(yscrollcommand=log_scrollbar.set)
    log_output.pack(side="left", fill="both", expand=True)
    log_scrollbar.pack(side="right", fill="y")

    def update_log_output():
        try:
            while True:
                message = log_queue.get_nowait()
                log_output.configure(state="normal")
                log_output.insert("end", f"{message}\n")
                log_output.see("end")
                log_output.configure(state="disabled")
        except queue.Empty:
            pass

        if not closing:
            root.after(100, update_log_output)

    def close_app():
        nonlocal closing
        if closing:
            return
        closing = True
        stop_event.set()
        stop_ngrok(ngrok_process)
        if server_socket is not None:
            server_socket.close()
        if server_thread is not None and server_thread.is_alive():
            server_thread.join(timeout=2)
        root.destroy()

    root.protocol("WM_DELETE_WINDOW", close_app)
    update_log_output()

    try:
        server_socket = create_server_socket(log)
    except OSError as error:
        status_var.set(f"Port {SERVER_PORT} is unavailable: {error}")
        root.mainloop()
        return

    server_thread = threading.Thread(target=start_server, args=(server_socket, stop_event, log, show_image), daemon=True)
    server_thread.start()

    try:
        ngrok_process = subprocess.Popen(
            ["ngrok", "tcp", str(SERVER_PORT)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            start_new_session=True,
        )
    except OSError as error:
        stop_event.set()
        server_socket.close()
        server_thread.join(timeout=2)
        status_var.set(f"Could not start ngrok: {error}")
        root.mainloop()
        return

    def wait_for_tunnel():
        nonlocal server_thread

        while not stop_event.is_set() and ngrok_process.poll() is None:
            try:
                tunnel = get_tcp_tunnel()
            except (OSError, ValueError, json.JSONDecodeError):
                tunnel = None

            if tunnel is not None:
                host, port = tunnel
                host_number = host.split(".", 1)[0]
                root.after(0, host_var.set, f"Host number: {host_number}")
                root.after(0, port_var.set, f"Port: {port}")
                root.after(0, status_var.set, f"Target host: {host}\nReady. Start the DSi scanner.")
                return

            time.sleep(0.25)

        if not stop_event.is_set():
            root.after(0, status_var.set, "ngrok stopped before a TCP tunnel was ready.")

    threading.Thread(target=wait_for_tunnel, daemon=True).start()

    try:
        root.mainloop()
    except KeyboardInterrupt:
        close_app()
    finally:
        close_app()


if __name__ == "__main__":
    run_app()