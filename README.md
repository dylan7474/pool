# Pool

A small cross-platform sample project for building SDL2-based games and applications. It demonstrates how to build the same C code for both Linux and Windows using Makefiles.

## Building

Run the configuration script to verify required tools and libraries:

```sh
./configure
```

### Linux

Build the native Linux binary using GNU Make:

```sh
make
```

### Windows (cross-compile)

Use MinGW and the provided Makefile to build a Windows executable from Linux:

```sh
make -f Makefile.win
```

## Controls

* **Arrow keys** – move the player
* **Esc** – quit the application

## Roadmap

* Flesh out gameplay and add additional levels
* Optional networking support via libcurl
* Embed assets directly into the executable
* Continuous integration for automated builds
