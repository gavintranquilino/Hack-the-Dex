# DSi QR Scanner Web Proxy

The runner starts the local TCP receiver and an ngrok TCP tunnel together. A
Tkinter window displays the current ngrok host number and port for the DSi.

## Setup

From this directory:

```bash
source .venv/bin/activate
pip install Pillow pyzbar playwright

# Install the Playwright browser once:
playwright install chromium
```

The system `zbar` library is also required by `pyzbar`. On Debian or Ubuntu:

```bash
sudo apt install libzbar0
```

The app connects to a normal Chromium session over CDP. Chromium uses a
dedicated persistent profile so login cookies and local storage survive every
app restart. Do not use your everyday Chromium profile for this.

### One-time browser setup

Run this once to create the dedicated profile and enable remote debugging:

```bash
mkdir -p "$HOME/snap/chromium/common/hackthedex-chrome"
chromium \
	--remote-debugging-port=9222 \
	--user-data-dir="$HOME/snap/chromium/common/hackthedex-chrome" \
	https://my.hackthenorth.com
```

Complete the login manually in this Chromium window. For Google login, use
this normal visible browser window rather than an automated login flow. After
the site is fully logged in, close Chromium normally so the profile is saved.

### Future runs

Use the same profile directory every time. Start Chromium with the same
command, then run the app in a separate terminal:

```bash
chromium \
	--remote-debugging-port=9222 \
	--user-data-dir="$HOME/snap/chromium/common/hackthedex-chrome" \
	https://my.hackthenorth.com

source .venv/bin/activate
python main.py
```

The app attaches to the existing Chromium session over CDP. It does not close
Chromium when the app exits. If the app starts Chromium automatically, it uses
the same profile directory. Do not delete or share this directory because it
contains authentication data. If the session expires, log in again in this
same profile and close Chromium normally to save the refreshed session.

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