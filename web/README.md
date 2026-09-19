# DSi QR Scanner Web Proxy

The runner starts the local TCP receiver and an ngrok TCP tunnel together. A
Tkinter window displays the current ngrok host number and port for the DSi.

## Setup

From this directory:

```bash
source .venv/bin/activate
pip install Pillow pyzbar
```

The system `zbar` library is also required by `pyzbar`. On Debian or Ubuntu:

```bash
sudo apt install libzbar0
```

Make sure ngrok is installed, authenticated, and available on `PATH`.

## Run

```bash
python main.py
```

Copy the displayed values into the DSi source:

```c
#define TARGET_HOST "<host number>.tcp.ngrok.io"
#define TARGET_PORT "<port>"
```

The displayed endpoint changes when the free ngrok tunnel restarts, so update
the DSi values each time the runner starts.