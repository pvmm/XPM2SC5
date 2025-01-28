# XPM2SC5

Converts XPM file into SCREEN5/V9990 (p1 mode) without any additional library other than libc. This project is meant to be used inside another, by a parent Makefile calling this Makefile inside a subdirectory. If the image is called `image.xpm`, run the code like this:

```
make images/image.raw
```
to create a raw VRAM file called `image.raw`. You may also create `.h` or `.c` files with the following commands:
```
make images/image.h
```
and
```
make images/image.c
```
respectively.

## extra parameters
The following command line parameters are recognised:
```
--screen5                Outputs a SCREEN5 compatible image (256 width max. with 15 colors).
--v9990                  Outputs a P1 mode compatible image (V9990 graphics card).
--keep-unused            Unused palette colors are still converted to output file.
--contains-palette       Get palette colors and order from first disposable line of the image.
--palette                Outputs palette too.
--image                  Outputs image too.
--header                 Outputs C-style header file.
--raw                    Outputs raw file (VRAM memory dump).
--basic                  Outputs BASIC file and respective BIN file.
--trans-color #<RRGGBB>  Supply hex string #RRGGBB as a transparent color replacement.
```
