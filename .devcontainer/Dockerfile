FROM ubuntu:22.04
ARG TARGETARCH=amd64
RUN apt-get update && apt-get install -y sudo openssl apt-utils

WORKDIR /workdir

# Define username and temporary uid and gid
ENV USER=holosis USER_ID=1000 USER_GID=1000

# Create user
RUN groupadd --gid "${USER_GID}" "${USER}" && \
    useradd \
      --uid ${USER_ID} \
      --gid ${USER_GID} \
      --create-home \
      --shell /bin/bash\
      --password $(openssl passwd -1 ubuntu)\
      ${USER}

# Setup locale
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get install -y locales && dpkg-reconfigure locales --frontend noninteractive && locale-gen "en_US.UTF-8" && update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

# Common packages for both architectures
RUN apt-get update && apt-get install -y \
    binfmt-support qemu qemu-user-static debootstrap kpartx \
    lvm2 dosfstools gpart binutils bison git libssl-dev gawk wget \
    git-core diffstat unzip texinfo build-essential chrpath socat \
    autoconf libtool libglib2.0-dev libarchive-dev xterm sed cvs subversion \
    kmod coreutils texi2html bc docbook-utils help2man make gcc g++ \
    desktop-file-utils libgl1-mesa-dev libglu1-mesa-dev mercurial automake groff curl \
    lzop asciidoc u-boot-tools mtd-utils device-tree-compiler flex \
    rsync cmake qemu-system-aarch64 \
    libncurses-dev python3-cryptography python3-jsonschema python3-pyelftools \
    python3-yaml swig gcc-arm-linux-gnueabihf gcc-aarch64-linux-gnu \
    libgnutls28-dev u-boot-tools vim nano tmux e2tools parted mtools cpio

# Install architecture-specific packages
RUN set -e && \
    dpkgArch="$(dpkg --print-architecture)" && \
    case "${dpkgArch##*-}" in \
        amd64) apt-get update && apt-get install -y gcc-multilib lib32ncurses5-dev ;; \
        arm64) echo "Running on ARM architecture - skipping x86-specific packages" ;; \
        *) echo "Unsupported architecture: ${dpkgArch}" && exit 1 ;; \
    esac

# Update to latest
RUN apt-get update && apt-get dist-upgrade -y

# Make /bin/sh symlink to bash instead of dash
RUN echo "dash dash/sh boolean false" | debconf-set-selections
RUN DEBIAN_FRONTEND=noninteractive dpkg-reconfigure dash

# Install the toolchain
RUN mkdir -p /opt/toolchain \
    && cd /opt/toolchain \
    && wget https://releases.linaro.org/components/toolchain/binaries/latest-7/aarch64-linux-gnu/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu.tar.xz \
    && tar -xvf gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu.tar.xz

COPY docker-entry.sh /usr/local/bin/
ENTRYPOINT ["docker-entry.sh"]
