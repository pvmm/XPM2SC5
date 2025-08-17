CC := gcc
LD := ld

TMP := $(basename $(notdir $(MAKECMDGOALS)))
EXT := $(suffix $(MAKECMDGOALS))

INPUT_FILE := $(basename $(MAKECMDGOALS)).xpm
$(info INPUT_FILE set to "$(INPUT_FILE)")

NAME := $(basename $(TMP))
$(info NAME set to "$(NAME)")

# get XPM_DATA field from file contents
FIELD := $(shell sed -n '/static/p' $(INPUT_FILE) | sed 's/^static\s\+char\s\+\*\s\+\(\w\+\).\+/\1/')
$(info Variable FIELD is set to "$(FIELD)")

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

BUILD_DIR ?= build
$(info BUILD_DIR is set to "$(BUILD_DIR)")
CFLAGS := $(CFLAGS) -I. -g
OUTPUT_FILE := $(BUILD_DIR)/$(NAME)
$(info OUTPUT_FILE is "$(OUTPUT_FILE)")
TARGET := $(BUILD_DIR)/$(NAME)
$(info TARGET is "$(TARGET)")

.PHONY: clean

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(MAKECMDGOALS): $(OUTPUT_FILE)

.DELETE_ON_ERROR:
$(OUTPUT_FILE): converter.c $(BUILD_DIR) $(INPUT_FILE)
	$(LD) -r -b binary rgb.txt -o rgb.o
	$(CC) $(CFLAGS) $(XPM_INCLUDE) -DXPM_DATA="$(FIELD)" -DXPM_LABEL="\"$(TAG_NAME)\"" -o $(OUTPUT_FILE) $< rgb.o

clean:
	rm -rf $(BUILD_DIR)
