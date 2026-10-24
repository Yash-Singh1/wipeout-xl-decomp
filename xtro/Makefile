# Byte-exact rebuild of the intro movie players XTRO1-4.EXE and NTSCANIM.EXE (Wipeout XL, USA)
# and PALANIM.EXE (Wipeout 2097, Europe). The six programs are one source built six times; they
# differ in the movie file name, its frame count and the video standard (see VARIANTS and
# REGION). Run via ./build.sh (Docker toolchain).
CROSS   ?= mips-linux-gnu-
AS      := $(CROSS)as
LD      := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy

# Six programs from one source: the movie file name, its frame count and the video standard
# are -D defines (REGION selects VERSION_US or VERSION_EU). The default target "players" builds
# all six. XTRO1 is built here; each other EXE uses a recursive make in its own object directory
# (build/<EXE>.obj) with its own defines, and is written to build/<EXE>.
.DEFAULT_GOAL := players

TARGET  := XTRO1.EXE
BUILD   := build
OUT     := build
REGION  := US
MOVIE_NAME   := xtro1
MOVIE_FRAMES := 630
VARIANTS := XTRO2.EXE XTRO3.EXE XTRO4.EXE NTSCANIM.EXE PALANIM.EXE
MOVIE_NAME_XTRO2.EXE := xtro2
MOVIE_NAME_XTRO3.EXE := xtro3
MOVIE_NAME_XTRO4.EXE := xtro4
MOVIE_NAME_NTSCANIM.EXE := make
MOVIE_NAME_PALANIM.EXE := make
MOVIE_FRAMES_XTRO2.EXE := 600
MOVIE_FRAMES_XTRO3.EXE := 780
MOVIE_FRAMES_XTRO4.EXE := 960
MOVIE_FRAMES_NTSCANIM.EXE := 2044
# The European player shows the same movie at 50 Hz: 2044 frames * 50 / 60, truncated.
MOVIE_FRAMES_PALANIM.EXE := 1703
REGION_PALANIM.EXE := EU
# The header's region string is the only header byte that differs between the two standards.
ifeq ($(REGION),EU)
HEADER_REGION := Europe area
else
HEADER_REGION := Japan area\\0
endif
ASFLAGS := -EL -march=r3000 -mtune=r3000 -mabi=32 -no-pad-sections -G0 -Iinclude
LDSCRIPT := $(BUILD)/$(TARGET).ld  # the five programs share one layout (XTRO1.EXE.ld)
LDFLAGS := -EL --no-check-sections -nostdlib \
           -T $(LDSCRIPT) -T undefined_syms_auto.txt -T undefined_funcs_auto.txt

# Same compiler as the game: PsyQ 3.6 CC1PSX ("GNU C 2.7.2.SN.1") under DOSBox.
CC1_SN  := tools/doscc1.sh 3.6
CPP     := tools/gcc/2.7.2/cpp
MASPSX  := python3 tools/maspsx/maspsx.py --aspsx-version=2.08 --expand-div --gp-allow-offset -G8 --use-comm-section
CPPFLAGS := -Iinclude -undef -D__GNUC__=2 -Dmips -D__mips__ -D__mips -Dpsx -D__psx__ \
            -D_LANGUAGE_C -DLANGUAGE_C -D_MIPSEL -nostdinc -lang-c \
            -DMOVIE_NAME=$(MOVIE_NAME) -DMOVIE_FRAMES=$(MOVIE_FRAMES) -DVERSION_$(REGION)
CFLAGS  := -quiet -O2 -G8 -mips1 -mcpu=3000 -mgas -msoft-float

S_FILES := $(shell find asm -name '*.s' -not -path 'asm/nonmatchings/*')
C_FILES := $(shell find src -name '*.c')
O_FILES := $(S_FILES:%=$(BUILD)/%.o) $(C_FILES:%=$(BUILD)/%.o)

players: all variants

all: $(OUT)/$(TARGET)

# Variants: a recursive make per EXE, with its own object directory and defines.
variants: $(VARIANTS)
XTRO1.EXE: all

$(VARIANTS):
	$(MAKE) TARGET=$@ BUILD=build/$@.obj OUT=build MOVIE_NAME=$(or $(MOVIE_NAME_$@),xtro1) \
		MOVIE_FRAMES=$(or $(MOVIE_FRAMES_$@),630) REGION=$(or $(REGION_$@),US) all

$(BUILD)/%.s.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

# The generated header is copied with its region string replaced (same 11 bytes either way).
# The US copy comes out identical to asm/header.s.
$(BUILD)/asm/header.s.o: asm/header.s
	@mkdir -p $(dir $@)
	sed 's/Japan area\\0/$(HEADER_REGION)/' $< > $(BUILD)/asm/header.s
	$(AS) $(ASFLAGS) -o $@ $(BUILD)/asm/header.s

FLAGS_STAMP := $(BUILD)/flags.$(shell echo '$(CPPFLAGS) $(CFLAGS)' | md5sum | cut -c1-8)
$(FLAGS_STAMP):
	@mkdir -p $(BUILD)
	@rm -f $(BUILD)/flags.*
	@touch $@

$(BUILD)/%.c.o: %.c $(FLAGS_STAMP) $(wildcard include/*.h)
	@mkdir -p $(dir $@)
	$(CPP) $(CPPFLAGS) $< -o $(BUILD)/$*.c.i
	$(CC1_SN) $(BUILD)/$*.c.i $(BUILD)/$*.c.s $(CFLAGS)
	sed -i 's/\r$$//' $(BUILD)/$*.c.s
	$(MASPSX) $(BUILD)/$*.c.s > $(BUILD)/$*.c.maspsx.s
	$(AS) $(ASFLAGS) -o $@ $(BUILD)/$*.c.maspsx.s

# XTRO1.EXE.ld names its objects as build/...; point them at this build's directory.
$(BUILD)/$(TARGET).ld: XTRO1.EXE.ld
	@mkdir -p $(BUILD)
	sed 's#build/#$(BUILD)/#g' $< > $@

$(BUILD)/$(TARGET).elf: $(O_FILES) $(LDSCRIPT)
	$(LD) $(LDFLAGS) -Map $(BUILD)/$(TARGET).map -o $@

# The file ends with zero padding up to the 0x20000-byte load size in the header.
$(OUT)/$(TARGET): $(BUILD)/$(TARGET).elf
	@mkdir -p $(OUT)
	$(OBJCOPY) -O binary -j .header $< $(BUILD)/$(TARGET).header
	$(OBJCOPY) -O binary -j .main $< $(BUILD)/$(TARGET).main
	cat $(BUILD)/$(TARGET).header $(BUILD)/$(TARGET).main > $@
	python3 tools/pad.py $@ 0x20800

clean:
	rm -rf $(BUILD)

# Compile one variant at a time.
.NOTPARALLEL: players variants
.PHONY: players all clean variants XTRO1.EXE $(VARIANTS)
