#!/bin/bash

# verify container user was set in dockerfile
if [ -z "${USER}" ]; then
  echo "Set user in Dockerfile";
  exit -1
fi

# verify host uid and gid passed in
if [ -z "${HOST_USER_ID}" -a -z "${HOST_USER_GID}" ]; then
    echo "Pass host uid and gid in docker run command" ;
    echo "e.g. -e HOST_USER_ID=$uid -e HOST_USER_GID=$gid" ;
    exit -2
fi

# replace uid and guid in /etc/passwd and /etc/group
sed -i -e "s/^${USER}:\([^:]*\):[0-9]*:[0-9]*/${USER}:\1:${HOST_USER_ID}:${HOST_USER_GID}/"  /etc/passwd
sed -i -e "s/^${USER}:\([^:]*\):[0-9]*/${USER}:\1:${HOST_USER_GID}/"  /etc/group

# Update ownership of the user's home directory
chown -R ${HOST_USER_ID}:${HOST_USER_GID} /home/${USER}

# Copy the .gitconfig file from the host to the container's user directory.
# This ensures that the container uses the same Git configuration as the host.
# Copying (instead of mounting) avoids potential access conflicts, especially
# when the file is being accessed or modified by multiple containers or the host simultaneously.
cp /tmp/host_gitconfig /home/holosis/.gitconfig
chown ${USER}:${USER} /home/holosis/.gitconfig

# allow user to run sudo
adduser ${USER} sudo

# Make sure current Linux Headers are installed
# Required for nxp-wlan-sdk yocto recipe
if [ ! -d "/lib/modules/$(uname -r)" ]; then
    echo ubuntu | sudo -S sudo apt install linux-headers-$(uname -r) -y
fi

#change to /workdir after login
echo "cd /workdir" > /home/${USER}/.bashrc

echo "export PATH=/opt/toolchain/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu/bin:${PATH}" >> /home/${USER}/.bashrc
echo "export CROSS_COMPILE=aarch64-linux-gnu-" >> /home/${USER}/.bashrc
echo "export CC=${CROSS_COMPILE}gcc" >> /home/${USER}/.bashrc
echo "CXX=${CROSS_COMPILE}g++" >> /home/${USER}/.bashrc


# If ENV_RUN_SCRIPT set in Docker Environment, run it after login
if [ ! "${ENV_RUN_SCRIPT}" = "" ]; then
    echo "${ENV_RUN_SCRIPT}" >> /home/${USER}/.bashrc
fi

# If a command was passed to docker run, execute it as the user. Otherwise, start a login shell.
if [ -n "$1" ]; then
    exec su - "${USER}" -c "$@"
else
    su - "${USER}"
fi
