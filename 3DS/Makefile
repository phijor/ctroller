#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#
# NO_SMDH: if set to anything, no SMDH file is generated.
# ROMFS is the directory which contains the RomFS, relative to the Makefile (Optional)
# APP_TITLE is the name of the app stored in the SMDH file (Optional)
# APP_DESCRIPTION is the description of the app stored in the SMDH file (Optional)
# APP_AUTHOR is the author of the app stored in the SMDH file (Optional)
# ICON is the filename of the icon (.png), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.png
#     - icon.png
#     - <libctru folder>/default_icon.png
#---------------------------------------------------------------------------------
TARGET		:=	ctroller
BUILD		:=  build
SOURCES		:=	source
DATA		:=	data
INCLUDES	:=	include
RESOURCES	:=	resources
ICON		:=	$(RESOURCES)/icon.png
BANNER		:=	$(RESOURCES)/banner.png
AUDIO		:=	$(RESOURCES)/audio.wav

VERSION_STRING	:=	$(shell git describe --tags --abbrev=0)
VERSION_MAJOR	:=	$(word 1,$(subst ., ,$(VERSION_STRING)))
VERSION_MINOR	:=	$(word 2,$(subst ., ,$(VERSION_STRING)))
VERSION_PATCH	:=	$(word 3,$(subst ., ,$(VERSION_STRING)))

APP_TITLE	:=	$(TARGET)
APP_DESCRIPTION	:=	Use your 3DS as PC gamepad
APP_AUTHOR	:=	phijor
APP_INFO	:=	$(RESOURCES)/AppInfo.rsf

DSHOSTNAME	:= n3ds
DSIP := $(shell getent hosts $(DSHOSTNAME) | awk '{print $$1}')

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -Wall -Wextra -O2 -mword-relocations \
			-fomit-frame-pointer -ffunction-sections -fshort-enums \
			$(ARCH)

CFLAGS	+=	$(INCLUDE)
CFLAGS  +=  -DARM11 -D_3DS \
			-DVERSION_MAJOR=$(VERSION_MAJOR) \
			-DVERSION_MINOR=$(VERSION_MINOR) \
			-DVERSION_PATCH=$(VERSION_PATCH)

CFLAGS_DEBUG	:=	 -DDEBUG
CFLAGS_RELEASE	:=	 -DNDEBUG -Werror

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -lctru

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(CTRULIB)


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))
SHLISTFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.shlist)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
			$(PICAFILES:.v.pica=.shbin.o) $(SHLISTFILES:.shlist=.shbin.o) \
			$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.png)
	ifneq (,$(findstring $(TARGET).png,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).png
	else
		ifneq (,$(findstring icon.png,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.png
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_SMDH)),)
	export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh
endif

ifneq ($(ROMFS),)
	export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
endif

.PHONY: $(BUILD) clean all run install dist

#---------------------------------------------------------------------------------
all: release

.PHONY: release
release: export CFLAGS += $(CFLAGS_RELEASE) $(INCLUDE)
release: $(OUTPUT).cia

#---------------------------------------------------------------------------------
.PHONY: debug
debug: export CFLAGS += $(CFLAGS_DEBUG) $(INCLUDE)
debug: export APP_TITLE := $(addsuffix -debug, $(APP_TITLE))
debug: $(BUILD)

#---------------------------------------------------------------------------------
bold = echo -e "\e[1m"$(1)"\e[0m"
#---------------------------------------------------------------------------------
$(BUILD):
	@$(call bold,"Compiling binaries...")
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) -e --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@$(call bold,"Cleaning...")
	@rm -fr $(BUILD) $(OUTPUT).{3dsx,smdh,elf,cia} \
		$(TARGET)-$(VERSION_STRING).tar{,.gz}

#---------------------------------------------------------------------------------
run: debug
	@$(call bold,"Running...")
	@3dslink -r 5 -a $(DSIP) $(OUTPUT).3dsx || true

#---------------------------------------------------------------------------------
install: release
	@$(call bold,"Uploading to $(DSIP)...")
	@./upload.sh $(DSIP) $(TARGET)

#---------------------------------------------------------------------------------
$(OUTPUT).cia: BANNER_TMP := $(BUILD)/$(notdir $(BANNER)).bnr
$(OUTPUT).cia: ICON_TMP := $(BUILD)/$(notdir $(ICON)).icn
$(OUTPUT).cia: $(BUILD)
	@$(call bold,"Building CIA...")
	@bannertool makebanner -i $(BANNER) -a $(AUDIO) -o $(BANNER_TMP)
	@bannertool makesmdh -s $(APP_TITLE) -l $(APP_TITLE) -p $(APP_AUTHOR) \
		-i $(ICON) -o $(ICON_TMP)
	@makerom -f cia -o $(OUTPUT).cia -rsf $(APP_INFO) \
		-DAPP_ENCRYPTED=false -DAPP_TITLE="$(APP_TITLE)" \
		-target t -exefslogo -elf $(OUTPUT).elf \
		-icon $(ICON_TMP) -banner $(BANNER_TMP)
	@echo "Created CIA \"$@\""

#---------------------------------------------------------------------------------
dist: release
	@$(call bold,"Creating distribution-tarball ($(VERSION_STRING))...")
	@tar -cvf $(TARGET)-$(VERSION_STRING).tar $(TARGET).{cia,3dsx,smdh} \
		../LICENSE ../README.md
	@gzip -f $(TARGET)-$(VERSION_STRING).tar

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
ifeq ($(strip $(NO_SMDH)),)
$(OUTPUT).3dsx	:	$(OUTPUT).elf $(OUTPUT).smdh
else
$(OUTPUT).3dsx	:	$(OUTPUT).elf
endif

$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
# rules for assembling GPU shaders
#---------------------------------------------------------------------------------
define shader-as
	$(eval CURBIN := $(patsubst %.shbin.o,%.shbin,$(notdir $@)))
	picasso -o $(CURBIN) $1
	bin2s $(CURBIN) | $(AS) -o $@
	echo "extern const u8" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(CURBIN) | tr . _)`.h
	echo "extern const u8" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(CURBIN) | tr . _)`.h
	echo "extern const u32" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(CURBIN) | tr . _)`.h
endef

%.shbin.o : %.v.pica %.g.pica
	@echo $(notdir $^)
	@$(call shader-as,$^)

%.shbin.o : %.v.pica
	@echo $(notdir $<)
	@$(call shader-as,$<)

%.shbin.o : %.shlist
	@echo $(notdir $<)
	@$(call shader-as,$(foreach file,$(shell cat $<),$(dir $<)/$(file)))

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
