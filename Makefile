CC := gcc
ASSETS_DIR := .

TMP := $(basename $(MAKECMDGOALS))
EXT := $(suffix $(MAKECMDGOALS))
INPUT_FILE := $(ASSETS_DIR)/$(TMP).xpm
NAME := $(basename $(TMP))

# get XPM_DATA field from file contents
FIELD := $(shell sed -n '/static/p' $(INPUT_FILE) | sed 's/^static\s\+char\s\+\*\s\+\(\w\+\).\+/\1/')

XPM_INCLUDE := -DXPM_FILE="\"$(INPUT_FILE)\""
TAG_NAME := $(guile (string-upcase "$(subst -,_,$(NAME))"))
CFLAGS := $(CFLAGS) -I.
BUILD_DIR := ./build
OUTPUT_FILE := $(BUILDDIR)/$(NAME)
TARGET := $(BUILDDIR)/$(NAME)

.PHONY: clean $(NAME).c $(NAME).h $(NAME).raw

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.DELETE_ON_ERROR:
$(NAME).h: converter.c $(INPUT_FILE) $(BUILD_DIR)
	echo BUILD_DIR=$(BUILD_DIR)
	$(CC) $(CFLAGS) $(XPM_INCLUDE) -DXPM_DATA="$(FIELD)" -DXPM_LABEL="\"$(guile (string-upcase "$(TAG_NAME)"))\"" -o $(BUILD_DIR)$(basename $(OUTPUT_FILE)) $<
	$(basename $(OUTPUT_FILE)) $(INPUT_FILE) > $(BUILD_DIR)$(OUTPUT_FILE).h
	rm $(BUILD_DIR)$(basename $(OUTPUT_FILE))

.DELETE_ON_ERROR:
$(NAME).c: converter.c $(INPUT_FILE) $(BUILD_DIR)
	$(CC) $(CFLAGS) $(XPM_INCLUDE) -DXPM_DATA="$(FIELD)" -DXPM_LABEL="\"$(guile (string-upcase "$(TAG_NAME)"))\"" -o $(BUILD_DIR)$(basename $(OUTPUT_FILE)) $<
	$(basename $(OUTPUT_FILE)) > $(BUILD_DIR)$(OUTPUT_FILE).c
	$(basename $(OUTPUT_FILE)) --header > $(BUILD_DIR)$(OUTPUT_FILE).h
	rm $(BUILD_DIR)$(basename $(OUTPUT_FILE))

.DELETE_ON_ERROR:
$(NAME).raw: converter.c $(INPUT_FILE) $(BUILD_DIR)
	echo BUILD_DIR=$(BUILD_DIR)
	$(CC) $(CFLAGS) $(XPM_INCLUDE) -DXPM_DATA="$(FIELD)" -DXPM_LABEL="\"$(guile (string-upcase "$(TAG_NAME)"))\"" -o $(BUILD_DIR)$(basename $(OUTPUT_FILE)) $<
	$(BUILD_DIR)$(basename $(OUTPUT_FILE)) --contains-palette --raw $(BUILD_DIR)$(OUTPUT_FILE).raw
	$(BUILD_DIR)$(basename $(OUTPUT_FILE)) --contains-palette --header > $(BUILD_DIR)$(OUTPUT_FILE).h
	rm $(BUILD_DIR)$(basename $(OUTPUT_FILE))

.DELETE_ON_ERROR:
$(NAME).bin: converter.c $(INPUT_FILE) $(BUILD_DIR)
	echo BUILD_DIR=$(BUILD_DIR)
	$(CC) $(CFLAGS) $(XPM_INCLUDE) -DXPM_DATA="$(FIELD)" -DXPM_LABEL="\"$(guile (string-upcase "$(TAG_NAME)"))\"" -o $(BUILD_DIR)$(basename $(OUTPUT_FILE)) $<
	$(BUILD_DIR)$(basename $(OUTPUT_FILE)) --contains-palette --basic --raw $(BUILD_DIR)$(OUTPUT_FILE).bin
	$(BUILD_DIR)$(basename $(OUTPUT_FILE)) --contains-palette --basic --palette > $(BUILD_DIR)$(OUTPUT_FILE).bas
	rm $(BUILD_DIR)$(basename $(OUTPUT_FILE))

clean:
	rm -rf $(BUILD_DIR)
