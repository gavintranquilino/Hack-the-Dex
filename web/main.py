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
from urllib.parse import urlsplit

from PIL import Image, ImageTk
from playwright.sync_api import Error as PlaywrightError
from playwright.sync_api import sync_playwright
from pyzbar.pyzbar import decode

SERVER_HOST = "0.0.0.0"
SERVER_PORT = 8080
EXPECTED_BYTES = 256 * 192
NGROK_API_URL = "http://127.0.0.1:4040/api/tunnels"
PLAYWRIGHT_CDP_URL = "http://127.0.0.1:9222"
CHROMIUM_PROFILE_DIR = os.path.join(
    os.path.expanduser("~"),
    "snap",
    "chromium",
    "common",
    "hackthedex-chrome",
)
ALLOWED_HOST = "my.hackthenorth.com"


def recv_exact(conn, size, timeout_seconds=1.0):
    conn.settimeout(timeout_seconds)
    chunks = bytearray()

    while len(chunks) < size:
        try:
            chunk = conn.recv(size - len(chunks))
        except socket.timeout:
            continue
        if not chunk:
            break
        chunks.extend(chunk)

    return bytes(chunks)


def send_json_response(conn, log, payload):
    response_bytes = json.dumps(payload, separators=(",", ":")).encode("utf-8")
    frame = len(response_bytes).to_bytes(4, byteorder="big", signed=False) + response_bytes
    try:
        conn.sendall(frame)
        log(f"-> Sent JSON response: {response_bytes.decode('utf-8')}")
    except OSError as error:
        log(f"-> Could not send JSON response: {error}")


def create_server_socket(log):
    log(f"Starting DSi Proxy Server on port {SERVER_PORT}...")

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.settimeout(1.0)
    server_socket.bind((SERVER_HOST, SERVER_PORT))
    server_socket.listen()
    return server_socket


def start_server(server_socket, stop_event, log, show_image, url_queue):
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

                try:
                    length_header = recv_exact(conn, 4, timeout_seconds=1.0)
                except OSError:
                    length_header = b""

                if len(length_header) != 4:
                    log("Error: Missing frame length header.")
                    send_json_response(conn, log, {"status": "error", "message": "missing_length_header"})
                    continue

                frame_size = int.from_bytes(length_header, byteorder="big", signed=False)
                if frame_size != EXPECTED_BYTES:
                    log(f"Error: Unexpected frame size {frame_size}, expected {EXPECTED_BYTES}.")
                    send_json_response(conn, log, {"status": "error", "message": "unexpected_frame_size", "expected": EXPECTED_BYTES, "got": frame_size})
                    continue

                data = recv_exact(conn, frame_size, timeout_seconds=1.0)
                if len(data) != frame_size:
                    log(f"Error: Incomplete frame ({len(data)} bytes).")
                    send_json_response(conn, log, {"status": "error", "message": "incomplete_frame"})
                    continue

                result = process_frame(data, log, show_image, url_queue.put)
                send_json_response(conn, log, result)


def process_frame(raw_bytes, log, show_image, open_url):
    image = Image.frombuffer("L", (256, 192), bytes(raw_bytes), "raw", "L", 0, 1)
    image.save("dsi_capture.png")
    show_image(image.copy())

    decoded_objects = decode(image)

    if not decoded_objects:
        log("-> No QR code detected. Check dsi_capture.png for focus/lighting.")
        return {"status": "error", "message": "no_qr_code_detected"}

    for obj in decoded_objects:
        qr_data = obj.data.decode("utf-8")
        log(f"-> QR Code Found: {qr_data}")

        parsed_url = urlsplit(qr_data)
        if parsed_url.scheme in ("http", "https") and parsed_url.hostname == ALLOWED_HOST:
            log("-> Opening in browser...")
            open_url(qr_data)
            return {
                "status": "ok",
                "message": "qr_accepted",
                "url": qr_data,
                "host": ALLOWED_HOST,
            }
        elif parsed_url.hostname:
            log(f"-> Ignoring URL from unapproved host: {parsed_url.hostname}")
            return {
                "status": "error",
                "message": "unapproved_host",
                "host": parsed_url.hostname,
            }
        else:
            log("-> Not a valid URL.")
            return {"status": "error", "message": "invalid_url"}

    return {"status": "error", "message": "qr_decode_failed"}


def playwright_worker(url_queue, stop_event, log):
    try:
        with sync_playwright() as playwright:
            browser = None
            chromium_process = None

            try:
                browser = playwright.chromium.connect_over_cdp(PLAYWRIGHT_CDP_URL)
                log("Connected to the existing Chromium session over CDP.")
            except PlaywrightError:
                log("No Chromium CDP session found. Starting Chromium...")
                os.makedirs(CHROMIUM_PROFILE_DIR, exist_ok=True)
                try:
                    chromium_process = subprocess.Popen(
                        [
                            "chromium",
                            "--remote-debugging-port=9222",
                            f"--user-data-dir={CHROMIUM_PROFILE_DIR}",
                            f"https://{ALLOWED_HOST}",
                        ],
                        stdout=subprocess.DEVNULL,
                        stderr=subprocess.DEVNULL,
                        start_new_session=True,
                    )
                except OSError as error:
                    log(f"Could not start Chromium: {error}")
                    return

            while browser is None and not stop_event.is_set():
                try:
                    browser = playwright.chromium.connect_over_cdp(PLAYWRIGHT_CDP_URL)
                except PlaywrightError:
                    if chromium_process is not None and chromium_process.poll() is not None:
                        log("Chromium exited before its CDP endpoint became ready.")
                        return
                    time.sleep(0.5)

            if browser is None:
                return

            contexts = browser.contexts
            if not contexts:
                log("Connected to Chromium, but no browser context is available.")
                return

            context = contexts[0]
            log("Connected to the authenticated Chromium session over CDP.")

            try:
                while not stop_event.is_set():
                    try:
                        url = url_queue.get(timeout=0.25)
                    except queue.Empty:
                        continue

                    try:
                        page = context.pages[0] if context.pages else context.new_page()
                        page.goto(url, wait_until="domcontentloaded")
                        log("-> Page opened in Playwright. No page parsing is enabled yet.")
                    except PlaywrightError as error:
                        log(f"-> Could not open page: {error}")
            finally:
                log("Disconnected from Chromium. The browser session remains open.")
    except PlaywrightError as error:
        log(f"Playwright error: {error}")
    except Exception as error:
        log(f"Could not start Playwright: {error}")


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
    url_queue = queue.Queue()
    stop_event = threading.Event()
    ngrok_process = None
    server_thread = None
    playwright_thread = None
    server_socket = None
    closing = False

    root = tk.Tk()
    root.title("DSi QR Scanner Connection")
    root.geometry("650x620")
    root.minsize(650, 620)
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

    def submit_manual_url():
        url = manual_url_var.get().strip()
        if not url:
            log("-> No manual URL entered.")
            return

        parsed_url = urlsplit(url)
        if parsed_url.scheme not in ("http", "https") or not parsed_url.netloc:
            log("-> Enter a valid http or https URL.")
            return

        if parsed_url.hostname == ALLOWED_HOST:
            log(f"-> Manual URL override: {url}")
        else:
            log(f"-> Opening debug URL from host {parsed_url.hostname or 'unknown'}: {url}")

        url_queue.put(url)

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
    log_frame.pack(fill="both", expand=True, padx=32, pady=(0, 8))
    log_output = tk.Text(log_frame, height=12, width=72, state="disabled", wrap="word")
    log_scrollbar = tk.Scrollbar(log_frame, command=log_output.yview)
    log_output.configure(yscrollcommand=log_scrollbar.set)
    log_output.pack(side="left", fill="both", expand=True)
    log_scrollbar.pack(side="right", fill="y")

    manual_frame = tk.Frame(root)
    manual_frame.pack(fill="x", padx=32, pady=(0, 18))
    manual_url_var = tk.StringVar(value="https://my.hackthenorth.com")
    tk.Label(manual_frame, text="Manual URL:", font=("TkDefaultFont", 10, "bold")).pack(anchor="w", pady=(0, 4))
    manual_entry = tk.Entry(manual_frame, textvariable=manual_url_var, width=70)
    manual_entry.pack(side="left", fill="x", expand=True)
    tk.Button(manual_frame, text="Open URL", command=submit_manual_url).pack(side="left", padx=(10, 0))
    manual_entry.bind("<Return>", lambda event: submit_manual_url())

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
        if playwright_thread is not None and playwright_thread.is_alive():
            playwright_thread.join(timeout=5)
        root.destroy()

    root.protocol("WM_DELETE_WINDOW", close_app)
    update_log_output()

    try:
        server_socket = create_server_socket(log)
    except OSError as error:
        status_var.set(f"Port {SERVER_PORT} is unavailable: {error}")
        root.mainloop()
        return

    server_thread = threading.Thread(
        target=start_server,
        args=(server_socket, stop_event, log, show_image, url_queue),
        daemon=True,
    )
    server_thread.start()

    playwright_thread = threading.Thread(
        target=playwright_worker,
        args=(url_queue, stop_event, log),
        daemon=True,
    )
    playwright_thread.start()

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