FROM ubuntu:latest
MAINTAINER m-wong <ming-liang.wong@uj.edu.pl>
ARG version="0.1"
LABEL description="SiFiCCDAQ"
LABEL version="${version}"

RUN apt -y update
RUN apt-get install -y software-properties-common
RUN DEBIAN_FRONTEND=noninteractive apt-get install tzdata
RUN apt -y install build-essential dpkg-dev cmake g++ binutils libx11-dev libxpm-dev libxft-dev libxext-dev libssl-dev python3 libpython3-dev python3-pip python3-pandas python3-matplotlib libboost-dev libboost-python-dev linux-image-$(uname -r) liblog4cxx-dev libiniparser-dev cmake xterm dkms python3-bitarray git tree libboost-regex-dev python3-requests python3-zmq
RUN sh INSTALL_DRIVERS.sh
WORKDIR /opt
COPY . /opt
RUN wget https://root.cern/download/root_v6.26.04.Linux-ubuntu20-x86_64-gcc9.4.tar.gz && tar -xzvf root_v6.26.04.Linux-ubuntu20-x86_64-gcc9.4.tar.gz && rm root_v6.26.04.Linux-ubuntu20-x86_64-gcc9.4.tar.gz && sh /opt/root_v6.26.04.Linux-ubuntu20-x86_64-gcc9.4/bin/thisroot.sh
RUN mkdir build && cd build && cmake3 -DCMAKE_BUILD_TYPE=Release .. && make
RUN tree -d
