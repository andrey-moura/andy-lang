<p align="center">
    <img src="resources/andy.svg" alt="Alt Text" style="width:200px; height:200px;">
</p>

# The Andy programming language

Andy is an interpreted object-oriented multi-purpose programming language

## Table of Contents
* [Examples](#Examples)
* [Availability](#Availability)
* [Install](#Install)
* [Install VSCode extension](#Install-VSCode-extension)
* [Building](#Building)
* [Building with UI enabled](#Building-with-UI-enabled)
* [The Language specification](./SPECIFICATION.md)
## Examples

If you want to run examples, try:

```sh
    andy examples/minimal.andy
```

This file has the content:

<pre style="background: #1e1e1e; color: #d4d4d4; padding: 1em; font-family: 'Fira Code', monospace; border-radius: 8px;">
<code>
<span style="color: #DCDCAA;">out</span> <span style="color: #CE9178;">'Hello from minimal!'</span>
</code>
</pre>

The result is:

```
    Hello from minimal!
```

## Availability

Environment | Build
--- | --- |
Ubuntu 22.04 | [![Ubuntu 22.04](https://github.com/andrey-moura/andy-lang/actions/workflows/build-ubuntu-22.04.yml/badge.svg?cache-control=no-cache)](https://github.com/andrey-moura/andy-lang/actions/workflows/build-ubuntu-22.04.yml)
Ubuntu 24.04 | [![Ubuntu 24.04](https://github.com/andrey-moura/andy-lang/actions/workflows/build-ubuntu-24.04.yml/badge.svg?cache-control=no-cache)](https://github.com/andrey-moura/andy-lang/actions/workflows/build-ubuntu-24.04.yml)
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

Download https://andy-lang.org/releases/andy-lang-installer/latest and run it.

### Install VSCode extension
Download the VSIX file from the https://andy-lang.org/releases/andy-lang-vscode/latest and follow the instructions available in the [Install from a VSIX](https://code.visualstudio.com/docs/configure/extensions/extension-marketplace#_install-from-a-vsix).

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

### Building with UI enabled


### Install SDL

#### Install Dependencies needed on Linux (use the package manager of your distribution)

```sh
    sudo apt install libx11-dev libxext-dev libwayland-dev libxrandr-dev libxi-dev libxinerama-dev libxcursor-dev libxfixes-dev
```

#### Building SDL

```sh
    git clone https://github.com/libsdl-org/SDL.git
    cd SDL/
    git checkout release-3.2.x
    cmake -DSDL_STATIC=ON -DSDL_SHARED=OFF -DCMAKE_BUILD_TYPE=Release -DSDL_AUDIO=OFF -DSDL_VIDEO=ON -DSDL_GPU=ON -DSDL_RENDER=ON -DSDL_CAMERA=OFF -DSDL_JOYSTICK=OFF -DSDL_HAPTIC=OFF -DSDL_HIDAPI=ON -DSDL_POWER=OFF -DSDL_SENSOR=OFF -DSDL_SENSOR=OFF -DSDL_DIALOG=OFF -B build .
    cmake --build build --config Release --parallel
```

After building, run as sudo on Linux or with an Administrator Command Prompt on Windows

```sh
    cmake --install build
```

Now follow the instructions in the [Building](#Building) section to build andy-lang. The Andy will automatically detect the SDL library and enable the UI.