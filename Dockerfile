FROM ubuntu:18.04

RUN apt update
RUN apt install -y software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt update
RUN apt install -y gcc-9 g++-9 git wget make

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 90

RUN wget https://github.com/Kitware/CMake/releases/download/v3.16.3/cmake-3.16.3-linux-x86_64.sh
RUN chmod +x cmake-3.16.3-linux-x86_64.sh
RUN ./cmake-3.16.3-linux-x86_64.sh --skip-license --prefix=/usr/local
RUN export PATH=/opt/cmake/bin:$PATH
RUN rm cmake-3.16.3-linux-x86_64.sh

COPY . /andy-lang

ENTRYPOINT ["bash", "-c", "\
  cd /andy-lang && \
  git pull && \
  cmake -DCMAKE_BUILD_TYPE=Release -B build . && \
  cmake --build build --config Release --parallel && \
  cp -r build ../andy-build"]