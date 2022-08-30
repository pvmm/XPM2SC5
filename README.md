# XPM2SC5

Converts XPM file into SCREEN5 without any additional library other than libc. This project is meant to be used inside another, by a parent Makefile calling this Makefile inside a subdirectory. If the image is called `image.xpm`, run the code like this:

```
XPM=image make image.raw
```
to create a raw VRAM file called `image.raw`. You may also create `.h` or `.c` files with the following commands:
```
XPM=image make image.h
```
and
```
XPM=image make image.c
```
respectively.
