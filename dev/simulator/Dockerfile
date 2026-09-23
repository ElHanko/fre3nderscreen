FROM debian:13-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        gosu \
        imagemagick \
        libsdl2-dev \
        rsync \
        xvfb \
    && rm -rf /var/lib/apt/lists/*

COPY entrypoint.sh /usr/local/bin/fre3nderscreen-dev
RUN chmod 0755 /usr/local/bin/fre3nderscreen-dev

WORKDIR /work/src
ENTRYPOINT ["/usr/local/bin/fre3nderscreen-dev"]
