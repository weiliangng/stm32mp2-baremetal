BUILDDIR = build
SHAREDDIR = ../shared

SOURCES := startup.s main.c

override LD       := aarch64-none-elf-gcc
override INCLUDES := -I.
override CFLAGS   += -ffreestanding -nostartfiles -fno-common \
                     -fdata-sections -ffunction-sections -c


include $(SHAREDDIR)/makefile-common.mk


# put this *after* the include of makefile-common.mk
.PHONY: list
list: $(ELF)
	$(OBJDMP) -d -S --prefix-addresses $< > $(BUILDDIR)/$(BINARYNAME).lst
	@echo "Wrote $(BUILDDIR)/$(BINARYNAME).lst"

