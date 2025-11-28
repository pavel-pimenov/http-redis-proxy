FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    libmicrohttpd-dev \
    libjsoncpp-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN g++ -std=c++17 -fpermissive -o l2-server main.cpp -lmicrohttpd -ljsoncpp -lpthread

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    libmicrohttpd12 \
    libjsoncpp25 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /root/
COPY --from=builder /app/l2-server .
EXPOSE 3000
CMD ["./l2-server"]
