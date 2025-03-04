<p align="center">
    <img src="resources/andy.svg" alt="Alt Text" style="width:200px; height:200px;">
</p>

# The Andy programming language

Andy is an interpreted object-oriented multi-purpose programming language

## Table of Contents
* [Examples](#Examples)
* [Availability](#Availability)
* [Install](#Install)
* [Building](#Building)

## Examples

If you want to run examples, try:

```sh
    andy examples/minimal.andy
```

This file has the content:

```typescript
    puts("Hello from minimal!");
```

The result is:

```
    Hello from minimal!
```

## Availability

Environment | Build
--- | --- |
Ubuntu 22.04 | [![Ubuntu 22.04](https://github.com/andrey-moura/andy-lang/actions/workflows/build-ubuntu-22.04.yml/badge.svg?cache-control=no-cache)](https://github.com/andrey-moura/andy-lang/actions/workflows/build-ubuntu-22.04.yml)
Ubuntu 20.04 | [![Ubuntu 20.04](https://github.com/andrey-moura/andy-lang/actions/workflows/build-ubuntu-24.04.yml/badge.svg?cache-control=no-cache)](https://github.com/andrey-moura/andy-lang/actions/workflows/build-ubuntu-24.04.yml)
Windows Server 2022 | [![Windows Server 2022](https://github.com/andrey-moura/andy-lang/actions/workflows/build-windows-2022.yml/badge.svg?cache-control=no-cache)](https://github.com/andrey-moura/andy-lang/actions/workflows/build-windows-2022.yml)
WebAssembly | [![WebAssembly](https://github.com/andrey-moura/andy-lang/actions/workflows/build-wasm.yml/badge.svg?cache-control=no-cache)](https://github.com/andrey-moura/andy-lang/actions/workflows/build-wasm.yml)

## Install

### Installation from andy-lang.org
#### Under Linux
```sh
    wget --content-disposition andy-lang.org/releases/andy-lang/latest
    sudo dpkg -i andy-lang-x.x.x.deb
```
#### Under Windows

Download andy-lang.org/releases/andy-lang-installer/latest and run it.

## Building

On Linux or Windows Developer Command Prompt

```sh
    git clone https://github.com/andrey-moura/andy-lang --recursive
    cd andy-lang
    cmake -DCMAKE_BUILD_TYPE=Release -B build .
    cmake --build build --config Release --parallel
```

After building, run as sudo on Linux or with an Administrator Command Prompt on Windows

```sh
    cmake --install build
```