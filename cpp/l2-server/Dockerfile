FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN g++ -std=c++17 -o l2-server main.cpp -lpthread

FROM ubuntu:22.04

WORKDIR /root/
COPY --from=builder /app/l2-server .
EXPOSE 3000
CMD ["./l2-server"]
