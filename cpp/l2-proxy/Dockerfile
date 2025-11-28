FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    libhiredis-dev \
    libcpprest-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN g++ -std=c++17 -o dmz-proxy main.cpp -lcpprest -lhiredis -lssl -lcrypto -lpthread

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y \
    libhiredis1.1.0 \
    libcpprest2.10 \
    libssl3t64 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /root/
COPY --from=builder /app/dmz-proxy .
EXPOSE 8080
CMD ["./dmz-proxy"]
