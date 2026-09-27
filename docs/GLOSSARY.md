# Glossary

Acronyms, abbreviations, tools, and project-specific terms used in this
repository's documentation and code.

| Term | Meaning |
| --- | --- |
| `.nds` file | The Nintendo DS executable image produced by the build. The main outputs are `hack_the_dex.nds`, `hack_the_dex_emu.nds`, and `hack_the_dex_offline.nds`. |
| `ARM7` / `ARM9` | The two ARM processor cores in Nintendo DS hardware. The application code runs on ARM9; the build also packages an ARM7 support core, including the DSWiFi support required by the online build. |
| BMP | Bitmap image format used for captured photos and drawings saved by the DS application. |
| BlocksDS | The open-source development toolchain and build system used to compile this project for Nintendo DS-family hardware. The main project Makefiles include BlocksDS rules for producing `.nds` files. |
| CDP | Chrome DevTools Protocol, the browser control protocol used by Playwright to connect to the already-running Chromium session on port `9222`. |
| Chromium | The browser used by the web companion to open the Hack the North website and maintain a dedicated authenticated profile. |
| cJSON | The small C JSON parser and generator included in [`app/`](../app/). It lets the DS read and validate profile responses. |
| DNS | The hostname lookup step that resolves the ngrok host into a network address before the DS opens its TCP connection. |
| Download Play | A Nintendo DS wireless feature for sending or sharing content between nearby DS systems. Sending files through Download Play is listed as future work and is not currently implemented. |
| DS | Nintendo's dual-screen handheld console family. In this project, "DS mode" means a future mode that reduces or removes features specific to DSi hardware, especially the camera content. |
| DSi | The Nintendo DSi revision of the DS. It adds cameras and other hardware used by the online Hack the Dex experience. |
| DLDI | The Dynamically Linked Device Interface used by DS flashcarts and related hardware to provide SD-card access. Emulator file behavior is kept separate because DLDI-backed writes may not be available or safe there. |
| DLDI write | A file write performed through the DS storage device interface. The normal hardware build uses these writes for saved profiles and images; the emulator build avoids some of them because emulator storage behavior differs. |
| DSWiFi | The BlocksDS/libnds Wi-Fi library used to initialize the DSi network hardware and make the application's TCP connection. |
| Emulator build | The application built with `EMU=1`. It is intended for a Nintendo DS emulator and uses emulator-specific paths and file behavior. |
| `EMU=1` | A compile-time Make variable that defines `EMU`, selects the emulator output name, skips the live setup screen, and uses emulator-safe profile behavior. |
| Hack the Dex | The Nintendo DS/DSi friendship and profile-sharing application built for Hack the North 2026. |
| JSON | JavaScript Object Notation, the text format used for profile data exchanged between the web companion and the DS and stored in `profile.json`. |
| libnds | The Nintendo DS programming library used by the application for hardware APIs, graphics, input, timing, and system services. |
| Make / Makefile | `make` is the command-line build tool. A Makefile describes how source files are compiled and linked. The project Makefile is [`app/Makefile`](../app/Makefile) and accepts the `EMU`, `OFFLINE`, and `BLOCKSDS` variables. |
| ngrok | A tunneling tool that exposes the desktop receiver through a temporary public TCP endpoint. The DS connects to the host and port displayed by the web companion. |
| ngrok host and port | The endpoint entered on the DS setup screen. A host such as `2.tcp.ngrok.io` identifies the tunnel and the port identifies the forwarded receiver connection. Free tunnel endpoints may change when ngrok restarts. |
| Offline build | The application built with `OFFLINE=1`. It skips Wi-Fi and QR profile retrieval and provides local profile entry for testing or use without the web companion. |
| Online build | The default application build. It uses the DSi camera and Wi-Fi to send a captured QR image to the desktop web companion and receive a profile response. |
| Playwright | A browser automation library used by the web companion to navigate the Hack the North site and read the profile associated with a scanned QR code. |
| Profile | A person's Hack the North information and associated assets. In the application it is represented primarily as JSON plus optional photo and signature image files. |
| Profile exchange | The complete interaction of scanning a friend's QR code, looking up their profile, adding a photo and drawing, and saving the result locally. |
| pyzbar / zbar | `pyzbar` is the Python wrapper used to decode QR and barcodes. It depends on the system `zbar` library, which must be installed separately on Linux. |
| QR code | A machine-readable square code scanned by the DSi camera. In this project it identifies the person whose Hack the North profile the web companion retrieves. |
| ROM | The packaged executable image for the console. In this repository, the final ROM is the generated `.nds` file. |
| SD card | The removable storage used by the DSi to persist profiles, photos, drawings, and signatures under `sd:/hackthedex`. |
| `sd:/hackthedex` | The root directory on the DS-visible SD card where profiles and captured assets are stored. |
| Snippet | An isolated experiment in [`snippets/`](../snippets/) for testing one feature, such as camera access, Wi-Fi, QR parsing, or drawing, before integrating it into the main application. |
| TCP | Transmission Control Protocol, the reliable connection-oriented protocol used between the DS and the Python receiver. |
| TCP frame | The image payload sent from the DS to the web companion. The receiver expects a length header followed by the camera frame bytes, then returns a length-prefixed profile response. |
| Tkinter | Python's standard desktop GUI toolkit. The web companion uses it to display the receiver and current ngrok endpoint status. |
| Web companion / web proxy | The Python program in [`web/`](../web/). It receives image data from the DS, uses a logged-in browser session to identify the profile, and sends profile data back over TCP. |
| Wi-Fi | The wireless network connection used by the DSi to reach the ngrok endpoint. |