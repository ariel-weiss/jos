FROM ubuntu:trusty

RUN sudo apt-get update
# Commands needed for QEMU install
RUN apt-get --no-install-recommends install -y gcc make build-essential gdb gcc-multilib zlib1g-dev libglib2.0-dev libpixman-1-dev
RUN apt-get install -y git && git clone https://github.com/geofft/qemu.git -b 6.828-1.7.0 && apt-get remove -y git
RUN cd qemu && ./configure --disable-kvm --target-list="i386-softmmu x86_64-softmmu" && make && make install && cd .. && rm -rf qemu
# Add JOS's code to the image
RUN mkdir jos
ADD . /usr/jos/
# Prepare for building jos
RUN cd /usr/jos
WORKDIR /usr/jos
