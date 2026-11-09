# Official BlocksDS image includes the Wonderful Toolchain and SDK.
FROM skylyrac/blocksds:slim-latest

USER root
RUN apt-get update \
    && apt-get install -y --no-install-recommends python3 python3-pip python3-venv curl ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# uv is useful for the Python companion service and other repo tooling.
RUN curl -LsSf https://astral.sh/uv/install.sh | sh \
    && install -m 0755 /root/.local/bin/uv /usr/local/bin/uv

WORKDIR /workspace
