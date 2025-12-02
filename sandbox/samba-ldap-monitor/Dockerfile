FROM debian:bullseye-slim

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
        samba \
        samba-common-bin \
        python3 \
        python3-pip \
        supervisor \
        nslcd \
        libnss-ldapd \
        libpam-ldapd \
        ldap-utils \
        libnss3-tools && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Установка экспортера
RUN pip3 install samba-exporter

# Копирование конфигов
COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
COPY entrypoint.sh /entrypoint.sh
COPY samba/smb.conf /etc/samba/smb.conf
COPY nslcd.conf.template /etc/nslcd.conf.template
COPY pam-samba /etc/pam.d/samba
COPY nsswitch.conf /etc/nsswitch.conf

RUN chmod +x /entrypoint.sh && \
    mkdir -p /mnt/share && \
    chown -R root:root /etc/samba && \
    chmod 600 /etc/nslcd.conf.template

VOLUME ["/mnt/share", "/var/log/samba"]
EXPOSE 139 445 9301

ENTRYPOINT ["/entrypoint.sh"]
