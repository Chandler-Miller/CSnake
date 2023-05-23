#Compiler and Linker
CC          := gcc

#The Directories, Source, Includes, Objects, Binary and Resources
SRCDIR      := src
INCDIR      := include
LIBDIR		:= /usr/local/lib
BUILDDIR    := build
TARGETDIR   := bin
SRCEXT      := c
DEPEXT      := d
OBJEXT      := o


#Flags, Libraries and Includes
CFLAGS      := -flto -Wall -Wextra -g3 -fsanitize=address \
			-fno-optimize-sibling-calls -fsanitize-address-use-after-scope \
            -fno-omit-frame-pointer -fsanitize=undefined \
            -Werror=vla -std=c2x \

LIB         := -lncurses -lm

ifeq ($(OS),Windows_NT)
#The Target Binary Program
TARGET := snake_game.exe
FIXPATH = $(subst /,\,$1)
RM			:= del /q /f
MD	:= mkdir
else
UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S), Linux)
	CFLAGS += -D LINUX
	endif
	ifeq ($(UNAME_S), Darwin)
	CFLAGS += -D OSX
	endif
TARGET := snake_game
RM = rm -f
MD	:= mkdir -p
endif
#---------------------------------------------------------------------------------
#DO NOT EDIT BELOW THIS LINE
#---------------------------------------------------------------------------------
SOURCES     := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS     := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))

INCLUDEDIRS	:= $(shell find $(INCDIR) -type d)
INCLUDES	:= $(patsubst %,-I%, $(INCLUDEDIRS:%/=%))

LIBDIRS 	:= $(shell find $(LIBDIR) -type d)
# LIBS		:= $(patsubst %,-L%, $(LIBDIRS:%/=%))
# LIBS 		:= -L/opt/homebrew/lib/ -L/usr/local/lib/

#Defauilt Make
all: $(TARGET)

#Remake
remake: cleaner all

#Make the Directories
directories:
	@mkdir -p $(TARGETDIR)
	@mkdir -p $(BUILDDIR)

#Clean only Objecst
clean:
	@$(RM) $(BUILDDIR)/*

#Full Clean, Objects and Binaries
cleaner: clean
	@$(RM) $(TARGETDIR)

#Pull in dependency info for *existing* .o files
-include $(OBJECTS:.$(OBJEXT)=.$(DEPEXT))

#Link
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGETDIR)/$(TARGET) $(LIBS) $^ $(LIB) -v

#Compile
$(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<
	@$(CC) $(CFLAGS) $(INCLUDES) -MM $(SRCDIR)/$*.$(SRCEXT) > $(BUILDDIR)/$*.$(DEPEXT)
	@cp -f $(BUILDDIR)/$*.$(DEPEXT) $(BUILDDIR)/$*.$(DEPEXT).tmp
	@sed -e 's|.*:|$(BUILDDIR)/$*.$(OBJEXT):|' < $(BUILDDIR)/$*.$(DEPEXT).tmp > $(BUILDDIR)/$*.$(DEPEXT)
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILDDIR)/$*.$(DEPEXT).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILDDIR)/$*.$(DEPEXT)
	@rm -f $(BUILDDIR)/$*.$(DEPEXT).tmp

#Run
run: all
	clear
	./$(TARGETDIR)/$(TARGET)

#Non-File Targets
.PHONY: all remake clean cleaner
