CC := gcc

TMP := $(basename $(notdir $(MAKECMDGOALS)))
EXT := $(suffix $(MAKECMDGOALS))
INPUT_FILE := $(basename $(MAKECMDGOALS)).xpm
$(info INPUT_FILE set to "$(INPUT_FILE)")
NAME := $(basename $(TMP))
$(info NAME set to "$(NAME)")

# trigger rule if output file is .c file
ifeq ($(MAKECMDGOALS),$(basename $(MAKECMDGOALS)).c)
    $(info MAKECMDGOALS = $(MAKECMDGOALS))
    $(info BASENAME = $(basename $(MAKECMDGOALS)))
    TARGET_c := $(basename $(MAKECMDGOALS)).c
    $(info TARGET is "$(TARGET_c)")
endif
# trigger rule if output file is .raw file
ifeq ($(MAKECMDGOALS),$(basename $(MAKECMDGOALS)).raw)
    $(info MAKECMDGOALS = $(MAKECMDGOALS))
    $(info BASENAME = $(basename $(MAKECMDGOALS)))
    TARGET_raw := $(basename $(MAKECMDGOALS)).raw
    $(info TARGET is "$(TARGET_raw)")
endif
# trigger rule if output file is .bin file
ifeq ($(MAKECMDGOALS),$(basename $(MAKECMDGOALS)).bin)
    $(info MAKECMDGOALS = $(MAKECMDGOALS))
    $(info BASENAME = $(basename $(MAKECMDGOALS)))
    TARGET_bin := $(basename $(MAKECMDGOALS)).bin
    $(info TARGET is "$(TARGET_bin)")
endif

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

BUILD_DIR ?= ./build
$(info BUILD_DIR is set to "$(BUILD_DIR)")
CFLAGS := $(CFLAGS) -I. -g
OUTPUT_FILE := $(BUILDDIR)/$(NAME)
TARGET := $(BUILDDIR)/$(NAME)

.PHONY: clean $(NAME).c $(NAME).h $(NAME).raw

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.DELETE_ON_ERROR:
$(TARGET_c): converter.c $(INPUT_FILE) $(BUILD_DIR)
	$(CC) $(CFLAGS) $(XPM_INCLUDE) -DXPM_DATA="$(FIELD)" -DXPM_LABEL="\"$(TAG_NAME)\"" -o $(BUILD_DIR)$(basename $(OUTPUT_FILE)) $<
#$(BUILD_DIR)$(basename $(OUTPUT_FILE)) > $(BUILD_DIR)$(OUTPUT_FILE).c
#$(BUILD_DIR)$(basename $(OUTPUT_FILE)) --header > $(BUILD_DIR)$(OUTPUT_FILE).h

.DELETE_ON_ERROR:
$(TARGET_raw): converter.c $(INPUT_FILE) $(BUILD_DIR)
	$(CC) $(CFLAGS) $(XPM_INCLUDE) -DXPM_DATA="$(FIELD)" -DXPM_LABEL="\"$(TAG_NAME)\"" -o $(BUILD_DIR)$(basename $(OUTPUT_FILE)) $<
#$(BUILD_DIR)$(basename $(OUTPUT_FILE)) --contains-palette --raw $(BUILD_DIR)$(OUTPUT_FILE).raw
#$(BUILD_DIR)$(basename $(OUTPUT_FILE)) --contains-palette --header > $(BUILD_DIR)$(OUTPUT_FILE).h

.DELETE_ON_ERROR:
$(TARGET_bin): converter.c $(INPUT_FILE) $(BUILD_DIR)
	$(info BUILD_DIR=$(BUILD_DIR))
	$(CC) $(CFLAGS) $(XPM_INCLUDE) -DXPM_DATA="$(FIELD)" -DXPM_LABEL="\"$(TAG_NAME)\"" -o $(BUILD_DIR)$(basename $(OUTPUT_FILE)) $<
#$(BUILD_DIR)$(basename $(OUTPUT_FILE)) --contains-palette --basic --raw $(BUILD_DIR)$(OUTPUT_FILE).bin
#$(BUILD_DIR)$(basename $(OUTPUT_FILE)) --contains-palette --basic --palette > $(BUILD_DIR)$(OUTPUT_FILE).bas

clean:
	rm -rf $(BUILD_DIR)
