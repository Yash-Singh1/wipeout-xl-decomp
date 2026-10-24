FROM --platform=linux/amd64 debian:bookworm-slim
ENV DEBIAN_FRONTEND=noninteractive
RUN dpkg --add-architecture i386 && apt-get update && \
    apt-get install -y --no-install-recommends wine wine32 libwine:i386 python3 make ca-certificates && \
    rm -rf /var/lib/apt/lists/*
ENV WINEPREFIX=/wine WINEARCH=win32 WINEDEBUG=-all
RUN wineboot --init >/dev/null 2>&1; wineserver -w; true
WORKDIR /work
