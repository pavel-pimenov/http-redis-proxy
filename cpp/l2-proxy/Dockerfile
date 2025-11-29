FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    libhiredis-dev \
    libssl-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY CMakeLists.txt main.cpp ./
COPY civetweb/ civetweb/
COPY jsoncpp/ jsoncpp/
COPY prometheus-cpp/ prometheus-cpp/
COPY sqlite/ sqlite/
RUN mkdir build && cd build && cmake .. && make

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y \
    libhiredis1.1.0 \
    libssl3t64 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /root/
COPY --from=builder /app/build/l2-proxy .
EXPOSE 8888
CMD ["./l2-proxy"]
