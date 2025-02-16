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
Ubuntu 20.04 | [![Ubuntu 20.04](https://github.com/andrey-moura/andy-lang/actions/workflows/build-ubuntu-20.04.yml/badge.svg?cache-control=no-cache)](https://github.com/andrey-moura/andy-lang/actions/workflows/build-ubuntu-20.04.yml)
Windows Server 2022 | [![Windows Server 2022](https://github.com/andrey-moura/andy-lang/actions/workflows/build-windows-2022.yml/badge.svg?cache-control=no-cache)](https://github.com/andrey-moura/andy-lang/actions/workflows/build-windows-2022.yml)
## Building

On Linux or Windows Developer Command Prompt

```sh
    git clone https://github.com/andrey-moura/uva
    cd uva
    cmake -B build .
    cmake --build build --config Release --parallel
    git clone https://github.com/andrey-moura/andy-lang
    cd andy-lang
    cmake -B build .
    cmake --build build --config Release --parallel
```

## Install
After building, run as sudo on Linux or with an Administrator Command Prompt on Windows

```sh
    cmake --install build
```
