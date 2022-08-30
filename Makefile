CC := gcc
ASSETS_DIR := .
INPUT_FILE := $(ASSETS_DIR)/$(XPM).xpm

# get XPM_DATA field from file contents
FIELD := $(shell sed -n '/static/p' $(INPUT_FILE) | sed 's/^static\s\+char\s\+\*\s\+\(\w\+\).\+/\1/')

XPM_INCLUDE := -DXPM_FILE="\"$(INPUT_FILE)\""
XPM2 := $(subst -,_,$(XPM))
CFLAGS := $(CFLAGS) -I.
BUILDDIR := .
OUTPUT_FILE := $(BUILDDIR)/$(XPM)
TARGET := $(BUILDDIR)/$(XPM)

.PHONY: clean $(XPM).c $(XPM).h $(XPM).raw

.DELETE_ON_ERROR:
$(XPM).h: xpm2sc5.c $(INPUT_FILE)
	$(CC) $(CFLAGS) $(XPM_INCLUDE) -DXPM_DATA="$(FIELD)" -DXPM_LABEL="\"$(guile (string-upcase "$(XPM2)"))\"" -o $(basename $(OUTPUT_FILE)) $<
	$(basename $(OUTPUT_FILE)) $(INPUT_FILE) > $(OUTPUT_FILE).h
	rm $(OUTPUT_FILE)

.DELETE_ON_ERROR:
$(XPM).c: xpm2sc5.c $(INPUT_FILE)
	$(CC) $(CFLAGS) $(XPM_INCLUDE) -DXPM_DATA="$(FIELD)" -DXPM_LABEL="\"$(guile (string-upcase "$(XPM2)"))\"" -o $(basename $(OUTPUT_FILE)) $<
	$(basename $(OUTPUT_FILE)) > $(OUTPUT_FILE).c
	$(basename $(OUTPUT_FILE)) --header > $(OUTPUT_FILE).h
	rm $(OUTPUT_FILE)

.DELETE_ON_ERROR:
$(XPM).raw: xpm2sc5.c $(INPUT_FILE)
	$(CC) $(CFLAGS) $(XPM_INCLUDE) -DXPM_DATA="$(FIELD)" -DXPM_LABEL="\"$(guile (string-upcase "$(XPM2)"))\"" -o $(basename $(OUTPUT_FILE)) $<
	$(basename $(OUTPUT_FILE)) --raw $(OUTPUT_FILE).raw
	$(basename $(OUTPUT_FILE)) --header > $(OUTPUT_FILE).h
	rm $(OUTPUT_FILE)
