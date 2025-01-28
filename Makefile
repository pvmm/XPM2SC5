CC := gcc
ASSETS_DIR := .

TMP := $(basename $(MAKECMDGOALS))
EXT := $(suffix $(MAKECMDGOALS))
INPUT_FILE := $(ASSETS_DIR)/$(TMP).xpm
NAME := $(basename $(TMP))
$(info NAME set to "$(NAME)")

# get XPM_DATA field from file contents
FIELD := $(shell sed -n '/static/p' $(INPUT_FILE) | sed 's/^static\s\+char\s\+\*\s\+\(\w\+\).\+/\1/')

# include xpm file in converter.c
XPM_INCLUDE := -DXPM_FILE="\"$(INPUT_FILE)\""

# create TAG_NAME variable
ifeq ($(findstring guile, $(.FEATURES)), guile)
    $(info Guile support found.)
    TAG_NAME := $(guile (string-upcase "$(subst -,_,$(NAME))"))
else
    $(info Guile support not found.)
    TAG_NAME := $(shell echo $(NAME) | tr 'a-z' 'A-Z')
endif

ifeq ($(TAG_NAME),)
    $(error TAG_NAME is empty)
else
    $(info TAG_NAME set to "$(TAG_NAME)")
endif

CFLAGS := $(CFLAGS) -I.
BUILD_DIR := ./build
OUTPUT_FILE := $(BUILDDIR)/$(NAME)
TARGET := $(BUILDDIR)/$(NAME)

.PHONY: clean $(NAME).c $(NAME).h $(NAME).raw

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.DELETE_ON_ERROR:
$(NAME).h: converter.c $(INPUT_FILE) $(BUILD_DIR)
	$(info BUILD_DIR=$(BUILD_DIR))
	$(CC) $(CFLAGS) $(XPM_INCLUDE) -DXPM_DATA="$(FIELD)" -DXPM_LABEL="\"$(TAG_NAME)\"" -o $(BUILD_DIR)$(basename $(OUTPUT_FILE)) $<
	$(BUILD_DIR)$(basename $(OUTPUT_FILE)) $(INPUT_FILE) > $(BUILD_DIR)$(OUTPUT_FILE).h
	rm $(BUILD_DIR)$(basename $(OUTPUT_FILE))

.DELETE_ON_ERROR:
$(NAME).c: converter.c $(INPUT_FILE) $(BUILD_DIR)
	$(CC) $(CFLAGS) $(XPM_INCLUDE) -DXPM_DATA="$(FIELD)" -DXPM_LABEL="\"$(TAG_NAME)\"" -o $(BUILD_DIR)$(basename $(OUTPUT_FILE)) $<
	$(BUILD_DIR)$(basename $(OUTPUT_FILE)) > $(BUILD_DIR)$(OUTPUT_FILE).c
	$(BUILD_DIR)$(basename $(OUTPUT_FILE)) --header > $(BUILD_DIR)$(OUTPUT_FILE).h
	rm $(BUILD_DIR)$(basename $(OUTPUT_FILE))

.DELETE_ON_ERROR:
$(NAME).raw: converter.c $(INPUT_FILE) $(BUILD_DIR)
	$(info BUILD_DIR=$(BUILD_DIR))
	$(CC) $(CFLAGS) $(XPM_INCLUDE) -DXPM_DATA="$(FIELD)" -DXPM_LABEL="\"$(TAG_NAME)\"" -o $(BUILD_DIR)$(basename $(OUTPUT_FILE)) $<
	$(BUILD_DIR)$(basename $(OUTPUT_FILE)) --contains-palette --raw $(BUILD_DIR)$(OUTPUT_FILE).raw
	$(BUILD_DIR)$(basename $(OUTPUT_FILE)) --contains-palette --header > $(BUILD_DIR)$(OUTPUT_FILE).h
	rm $(BUILD_DIR)$(basename $(OUTPUT_FILE))

.DELETE_ON_ERROR:
$(NAME).bin: converter.c $(INPUT_FILE) $(BUILD_DIR)
	$(info BUILD_DIR=$(BUILD_DIR))
	$(CC) $(CFLAGS) $(XPM_INCLUDE) -DXPM_DATA="$(FIELD)" -DXPM_LABEL="\"$(TAG_NAME)\"" -o $(BUILD_DIR)$(basename $(OUTPUT_FILE)) $<
	$(BUILD_DIR)$(basename $(OUTPUT_FILE)) --contains-palette --basic --raw $(BUILD_DIR)$(OUTPUT_FILE).bin
	$(BUILD_DIR)$(basename $(OUTPUT_FILE)) --contains-palette --basic --palette > $(BUILD_DIR)$(OUTPUT_FILE).bas
	rm $(BUILD_DIR)$(basename $(OUTPUT_FILE))

clean:
	rm -rf $(BUILD_DIR)
