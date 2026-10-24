# Byte-exact rebuild of SCUS_943.51 (Wipeout XL, USA) or, with VERSION=eu, of SLES_003.27
# (Wipeout 2097, Europe): the same sources, built with -DVERSION_EU.
# Run via ./build.sh [eu] (uses the Docker toolchain) or directly on a host with mips-linux-gnu binutils.
VERSION ?= us
.DEFAULT_GOAL := all
CROSS   ?= mips-linux-gnu-
AS      := $(CROSS)as
LD      := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy

ifeq ($(VERSION),eu)
TARGET  := SLES_003.27
BUILD   ?= build/eu
LDBUILD := build/eu
PADSIZE := 0x86000
UNDEF_SYMS  := undefined_syms_auto_eu.txt
UNDEF_FUNCS := undefined_funcs_auto_eu.txt
EXTRA_LD    := -T config/eu_link_syms.ld
VERSION_DEF := -DVERSION_EU
else ifeq ($(VERSION),us)
TARGET  := SCUS_943.51
BUILD   ?= build
LDBUILD := build
PADSIZE := 0x85800
UNDEF_SYMS  := undefined_syms_auto.txt
UNDEF_FUNCS := undefined_funcs_auto.txt
EXTRA_LD    :=
VERSION_DEF := -DVERSION_US
else
$(error VERSION must be us or eu)
endif
ASFLAGS := -EL -march=r3000 -mtune=r3000 -mabi=32 -no-pad-sections -G0 -Iinclude
# The linker script names objects under $(LDBUILD)/; another BUILD dir gets a rewritten copy.
ifeq ($(BUILD),$(LDBUILD))
LDSCRIPT := $(TARGET).ld
else
LDSCRIPT := $(BUILD)/$(TARGET).ld
endif
LDFLAGS := -EL --no-check-sections -nostdlib \
           -T $(LDSCRIPT) -T $(UNDEF_SYMS) -T $(UNDEF_FUNCS) $(EXTRA_LD)

GCC_VER ?= 2.7.2
# XL was built with PsyQ 3.6's CC1PSX ("GNU C 2.7.2.SN.1"), run under DOSBox.
# The decompals gcc builds differ from it in register allocation/scheduling.
CC1_SN  := tools/doscc1.sh 3.6
CPP     := tools/gcc/$(GCC_VER)/cpp
CC1     := tools/gcc/$(GCC_VER)/cc1
MASPSX  := python3 tools/maspsx/maspsx.py --aspsx-version=2.08 --expand-div --gp-allow-offset -G8 --use-comm-section
CPPFLAGS := -Iinclude -undef -D__GNUC__=2 -Dmips -D__mips__ -D__mips -Dpsx -D__psx__ \
            -D_LANGUAGE_C -DLANGUAGE_C -D_MIPSEL -nostdinc -lang-c $(VERSION_DEF)
CFLAGS  := -quiet -O2 -G8 -mips1 -mcpu=3000 -fgnu-linker -mgas -msoft-float

ifeq ($(VERSION),eu)
# The EU split lives in asm/eu.  Every file in src/ is built with -DVERSION_EU; the PsyQ
# libraries are linked from their split asm in asm/eu/text/.
EU_C      := $(basename $(notdir $(wildcard src/*.c)))
EU_ALL    := $(basename $(notdir $(wildcard asm/eu/text/*.s)))
EU_ASM    := $(filter-out $(EU_C),$(EU_ALL))
S_FILES   := $(shell find asm/eu -name '*.s' -not -path 'asm/eu/nonmatchings/*' -not -path 'asm/eu/text/*')
C_FILES   := $(EU_C:%=src/%.c)
O_FILES   := $(S_FILES:%=$(BUILD)/%.o) $(C_FILES:%=$(BUILD)/%.o) $(EU_ASM:%=$(BUILD)/src/%.c.o)
ASM_ROOT_SED = sed -i 's|asm/nonmatchings/|asm/eu/nonmatchings/|g' $(BUILD)/$*.c.s
$(EU_ASM:%=$(BUILD)/src/%.c.o): $(BUILD)/src/%.c.o: asm/eu/text/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<
$(EU_ALL:%=$(BUILD)/src/%.c.o): $(BUILD)/src/%.c.o: $(BUILD)/src/%.origin
$(EU_C:%=$(BUILD)/src/%.origin): FORCE
	@mkdir -p $(dir $@); echo c > $@.new; cmp -s $@.new $@ && rm $@.new || mv $@.new $@
$(EU_ASM:%=$(BUILD)/src/%.origin): FORCE
	@mkdir -p $(dir $@); echo asm > $@.new; cmp -s $@.new $@ && rm $@.new || mv $@.new $@
FORCE:
else
# asm/nonmatchings/ holds per-function asm pulled in by INCLUDE_ASM from src/.
S_FILES := $(shell find asm -name '*.s' -not -path 'asm/nonmatchings/*' -not -path 'asm/eu/*')
C_FILES := $(shell find src -name '*.c')
O_FILES := $(S_FILES:%=$(BUILD)/%.o) $(C_FILES:%=$(BUILD)/%.o)
ASM_ROOT_SED = true
endif

all: $(BUILD)/$(TARGET)

$(BUILD)/%.s.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

# Rebuild C objects whenever the compiler or flags change.
FLAGS_STAMP := $(BUILD)/flags.$(shell echo '$(GCC_VER) $(CPPFLAGS) $(CFLAGS)' | md5sum | cut -c1-8)
$(FLAGS_STAMP):
	@mkdir -p $(BUILD)
	@rm -f $(BUILD)/flags.*
	@touch $@

$(BUILD)/%.c.o: %.c $(FLAGS_STAMP) $(wildcard include/*.h)
	@mkdir -p $(dir $@)
	$(CPP) $(CPPFLAGS) $< -o $(BUILD)/$*.c.i
	$(CC1_SN) $(BUILD)/$*.c.i $(BUILD)/$*.c.s $(filter-out -fgnu-linker,$(CFLAGS))
	sed -i 's/\r$$//' $(BUILD)/$*.c.s
	$(ASM_ROOT_SED)
	$(MASPSX) $(BUILD)/$*.c.s > $(BUILD)/$*.c.maspsx.s
	$(AS) $(ASFLAGS) -o $@ $(BUILD)/$*.c.maspsx.s

ifneq ($(BUILD),$(LDBUILD))
$(LDSCRIPT): $(TARGET).ld
	@mkdir -p $(BUILD)
	sed 's|\b$(LDBUILD)/|$(BUILD)/|g' $< > $@
endif

$(BUILD)/$(TARGET).elf: $(O_FILES) $(LDSCRIPT)
	$(LD) $(LDFLAGS) -Map $(BUILD)/$(TARGET).map -o $@

$(BUILD)/$(TARGET): $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary -j .header $< $@.header
	$(OBJCOPY) -O binary -j .pre $< $@.pre
	$(OBJCOPY) -O binary -j .main $< $@.main
	cat $@.header $@.pre $@.main > $@
	python3 tools/pad.py $@ $(PADSIZE)

clean:
	rm -rf $(BUILD)

.PHONY: all clean
