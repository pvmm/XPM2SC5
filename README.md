# XPM2SC5

Converts XPM file into SCREEN5/V9990 (p1 mode) without any additional library other than libc. This project is meant to be used inside another, by a parent Makefile calling this Makefile inside a subdirectory. If the image is called `image1.xpm`, run the code like this:
```
make images/image1.raw
build/image1 --raw build/image1.raw
build/image1 --header > build/image1.h
```
to create a raw VRAM file called `image2.raw`. You may also create `.c` and `.h` files with the following commands:
```
make images/image2.c
build/image2 > build/image2.c
build/image2 --header > build/image2.h
```

## extra parameters
The binary generated by `converter.c` recognise these command line parameters:
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
