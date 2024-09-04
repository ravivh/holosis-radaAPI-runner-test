# Use an official Ubuntu base image
FROM --platform=linux/amd64 ubuntu:20.04
# https://stackoverflow.com/questions/71040681/qemu-x86-64-could-not-open-lib64-ld-linux-x86-64-so-2-no-such-file-or-direc

# Set the working directory in the container
WORKDIR /usr/src/app


RUN apt-get update && apt-get install -y software-properties-common && \
    add-apt-repository universe && \
    apt-get update

USER root

# Install necessary packages
RUN DEBIAN_FRONTEND=noninteractive apt-get update && apt-get install -y \
    g++ \
    make \
    cmake \
    libjson-c-dev \
    libxml2-dev \
    git \
    curl \
    ssh \
    gdb \
    ca-certificates \
    sudo \
    locales \
    build-essential \
    libc6-dev-armhf-cross \
    libstdc++6 \
    wget \
    xz-utils \
    clang \
    && add-apt-repository ppa:deadsnakes/ppa \
    && apt-get update && apt-get install -y \
    python3.5 \
    python3.5-dev \
    # any other tools you might need
    && apt-get clean \
    # Add any other libraries your project depends on
    && rm -rf /var/lib/apt/lists/*

# Install pip for Python 3.5
RUN apt-get update && apt-get install -y \
    python3.5-venv \
    && curl https://bootstrap.pypa.io/pip/3.5/get-pip.py -o get-pip.py \
    && python3.5 get-pip.py \
    && rm get-pip.py

# Set Python 3.5 as the default python3
RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.5 1

# Verify the installation
RUN python3 --version && python3 -m pip --version

# linaro for compiling shared libraries on ARM device
RUN wget https://releases.linaro.org/components/toolchain/binaries/latest-6/arm-linux-gnueabihf/gcc-linaro-6.4.1-2018.05-x86_64_arm-linux-gnueabihf.tar.xz \
    && tar -xf gcc-linaro-6.4.1-2018.05-x86_64_arm-linux-gnueabihf.tar.xz -C /opt \
    && rm gcc-linaro-6.4.1-2018.05-x86_64_arm-linux-gnueabihf.tar.xz


# Clone osxcross
# RUN git clone https://github.com/tpoechtrager/osxcross.git

# Set the locale
USER root
RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

ENV PATH="/opt/gcc-linaro-6.4.1-2018.05-x86_64_arm-linux-gnueabihf/bin:${PATH}"
ENV CROSS_COMPILE=arm-linux-gnueabihf-
ENV CC="${CROSS_COMPILE}gcc"
ENV CXX="${CROSS_COMPILE}g++"
# Set default user, when opening the container
# Create a non-root user to use with VS Code
# RUN useradd -m developer -s /bin/bash \
#     && echo "developer:developer" | chpasswd \
#     && adduser developer sudo

# Configure SSH
# RUN mkdir /home/developer/.ssh \
#     && chmod 700 /home/developer/.ssh \
#     && ssh-keyscan github.com >> /home/developer/.ssh/known_hosts \
#     && chown -R developer:developer /home/developer/.ssh

# Copy the code to the container's workspace
#COPY . /usr/src/app