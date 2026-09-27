# Development Snippets

These are small BlocksDS programs used to test individual pieces of the
Hack the Dex hardware and network workflow before integrating them into
`app/`.

- `blockds_camera_test/`: camera and BlocksDS setup experiments.
- `hello_world_dsi_blockds/`: minimal DSi/BlocksDS starter project.
- `network_scan/`: DSi network scanning and TCP connectivity experiment.
- `ping/`: basic hostname and Wi-Fi connection test; see its
  [`README.md`](ping/README.md).
- `qr_code_parse/`: QR/profile endpoint parsing and TCP response experiment.
- `signature/`: standalone drawing/signature experiment.

Each snippet has its own Makefile and should be built from its own directory.
They are exploratory programs, not additional release targets for the main
application.