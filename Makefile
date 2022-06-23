ifdef ($(XPM))
    XPM := $(XPM)
else
    XPM := image.xpm
endif

XPM_INCLUDE := -DXPM_FILE="\"$(XPM)\""
XPM_DATA := -DXPM_DATA=$(subst .,_,$(XPM))
CC := gcc

xpm2sc5: xpm2sc5.c
	$(CC) $(XPM_INCLUDE) $(XPM_DATA) -o xpm2sc5 xpm2sc5.c

