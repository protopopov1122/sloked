FROM ubuntu:focal

ENV DEBIAN_FRONTEND=noninteractive
RUN apt update -y
# Toolchains
RUN apt install -y gcc g++ clang
# Build tools
RUN apt install -y meson pkg-config
# Dependencies
RUN apt install -y zlib1g-dev libssl-dev libncurses-dev liblua5.3-dev
# NodeJS
RUN apt install -y nodejs npm
# Utilities
RUN apt install -y wget

# Common structure
ENV PROJECT_ROOT=/project/sloked
RUN mkdir -p $PROJECT_ROOT
RUN mkdir -p $PROJECT_ROOT/src
RUN mkdir -p $PROJECT_ROOT/third-party/catch2
RUN wget "https://github.com/catchorg/Catch2/releases/download/v2.12.1/catch.hpp" -O $PROJECT_ROOT/third-party/catch2/catch.hpp
RUN mkdir -p $PROJECT_ROOT/bin/gcc
RUN mkdir -p $PROJECT_ROOT/bin/clang

# User configuration
RUN groupadd sloked && useradd sloked -g sloked -m
RUN chown -R sloked:sloked $PROJECT_ROOT
USER sloked