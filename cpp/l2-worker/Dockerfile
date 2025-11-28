FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    libhiredis-dev \
    libcurl4-openssl-dev \
    libjsoncpp-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN g++ -std=c++17 -o l2-worker main.cpp -lhiredis -lcurl -ljsoncpp -lpthread

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y \
    libhiredis-dev \
    libcurl4-openssl-dev \
    libjsoncpp-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /root/
COPY --from=builder /app/l2-worker .
CMD ["./l2-worker"]
