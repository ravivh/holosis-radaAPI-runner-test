
FROM ubuntu:22.04

RUN apt-get update
RUN apt-get install -y sudo openssl apt-utils

WORKDIR /workdir

# Define username and temporary uid and gid
ENV USER=holosis USER_ID=1000 USER_GID=1000

# now creating user, change password to 'ubuntu'
RUN groupadd --gid "${USER_GID}" "${USER}" && \
    useradd \
      --uid ${USER_ID} \
      --gid ${USER_GID} \
      --create-home \
      --shell /bin/bash\
      --password $(openssl passwd -1 ubuntu)\
      ${USER}

#setup locale
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get install -y locales && dpkg-reconfigure locales --frontend noninteractive && locale-gen "en_US.UTF-8" && update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

# Debian
RUN apt-get update && apt-get install -y \
     binfmt-support qemu qemu-user-static debootstrap kpartx \
     lvm2 dosfstools gpart binutils bison git lib32ncurses5-dev libssl-dev gawk wget \
     git-core diffstat unzip texinfo gcc-multilib build-essential chrpath socat libsdl1.2-dev \
     autoconf libtool libglib2.0-dev libarchive-dev xterm sed cvs subversion \
     kmod coreutils texi2html bc docbook-utils help2man make gcc g++ \
     desktop-file-utils libgl1-mesa-dev libglu1-mesa-dev mercurial automake groff curl \
     lzop asciidoc u-boot-tools mtd-utils device-tree-compiler flex \
     rsync cmake qemu-system-aarch64


RUN curl https://storage.googleapis.com/git-repo-downloads/repo > /bin/repo && chmod a+rx /bin/repo

# Install development utilities
RUN apt-get update && apt-get install -y \
     vim nano tmux

# Install development packages
RUN apt-get update && apt-get install -y \
     python3-cryptography python3-jsonschema python3-pyelftools \
     python3-yaml swig gcc-arm-linux-gnueabihf gcc-aarch64-linux-gnu \
     libgnutls28-dev u-boot-tools

# Update to latest
RUN apt-get update && apt-get dist-upgrade -y

# make /bin/sh symlink to bash instead of dash:
RUN echo "dash dash/sh boolean false" | debconf-set-selections
RUN DEBIAN_FRONTEND=noninteractive dpkg-reconfigure dash

# additional packages
RUN apt-get install -y e2tools parted mtools
RUN apt-get install -y cpio


#install the toolchain
RUN mkdir -p /opt/toolchain \
    && cd /opt/toolchain \
    && wget https://releases.linaro.org/components/toolchain/binaries/latest-7/aarch64-linux-gnu/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu.tar.xz \
    && tar -xvf gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu.tar.xz


COPY docker-entry.sh /usr/local/bin/
ENTRYPOINT ["docker-entry.sh"]
