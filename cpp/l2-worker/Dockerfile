FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    pkg-config \
    libhiredis-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Гарантируем наличие jsoncpp.pc
RUN if [ ! -f /usr/lib/x86_64-linux-gnu/pkgconfig/jsoncpp.pc ]; then \
      echo "prefix=/usr" > /usr/lib/x86_64-linux-gnu/pkgconfig/jsoncpp.pc && \
      echo "exec_prefix=\${prefix}" >> /usr/lib/x86_64-linux-gnu/pkgconfig/jsoncpp.pc && \
      echo "libdir=\${exec_prefix}/lib/x86_64-linux-gnu" >> /usr/lib/x86_64-linux-gnu/pkgconfig/jsoncpp.pc && \
      echo "includedir=\${prefix}/include" >> /usr/lib/x86_64-linux-gnu/pkgconfig/jsoncpp.pc && \
      echo "" >> /usr/lib/x86_64-linux-gnu/pkgconfig/jsoncpp.pc && \
      echo "Name: jsoncpp" >> /usr/lib/x86_64-linux-gnu/pkgconfig/jsoncpp.pc && \
      echo "Description: JSON C++ library" >> /usr/lib/x86_64-linux-gnu/pkgconfig/jsoncpp.pc && \
      echo "Version: 1.9.5" >> /usr/lib/x86_64-linux-gnu/pkgconfig/jsoncpp.pc && \
      echo "Libs: -ljsoncpp" >> /usr/lib/x86_64-linux-gnu/pkgconfig/jsoncpp.pc && \
      echo "Cflags: -I\${includedir}/jsoncpp" >> /usr/lib/x86_64-linux-gnu/pkgconfig/jsoncpp.pc; \
    fi

WORKDIR /app
COPY CMakeLists.txt main.cpp ./
COPY civetweb/ civetweb/
COPY jsoncpp/ jsoncpp/
COPY prometheus-cpp/ prometheus-cpp/
RUN mkdir build && cd build && cmake .. && make

FROM ubuntu:24.04
#RUN apt-cache search hiredis
RUN apt-get update && apt-get install -y \
    libhiredis1.1.0 \
    libcurl4 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /root/
COPY --from=builder /app/build/l2-worker .
CMD ["./l2-worker"]