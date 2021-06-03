#########################
#Main Makefile
#########################
CCW32 = i686-w64-mingw32-gcc
CCW64 = x86_64-w64-mingw32-gcc
CCL = gcc
CFLAGS = -w -g
LIBSL = -lmenu -lpanel -lform -lncurses
LIBSM =	-lmenu -lpanel -lform -lncurses
LIBSW32 = -L/usr/lib/x86_64-linux-gnu/ -lmenu -lpanel -lform -lncurses
LIBSW64 = -L/usr/lib/x86_64-linux-gnu/ -lmenu -lpanel -lform -lncurses
EXTL = .pe
EXTM = .mac
EXTW32 = .x32.exe
EXTW64 = .x64.exe

#Get OS and configure based on OS
ifeq ($(OS),Windows_NT)
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        CFLAGSW += -D AMD64
   		DISTRO = windows64
   	else
	    ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
	        CFLAGSW += -D AMD64
			DISTRO = windows64
	    endif
	    ifeq ($(PROCESSOR_ARCHITECTURE),x86)
	        CFLAGSW += -D IA32 WIN32
			DISTRO = windows32
	    endif
    endif
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        CFLAGSL += -D LINUX
   		DISTRO = linux
    endif
    ifeq ($(UNAME_S),Darwin)
        CFLAGSL += -D OSX
   		DISTRO = mac
    endif
    ifeq ($(UNAME), Solaris)
   		DISTRO = solaris
    endif
    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)
        CFLAGSL += -D AMD64
    endif
    ifneq ($(filter %86,$(UNAME_P)),)
        CFLAGSL += -D IA32
    endif
    ifneq ($(filter arm%,$(UNAME_P)),)
        CFLAGSL += -D ARM
    endif
endif

.PHONY: main all deploy help fix linux mac solaris windows32 windows64 run cl clean clean-linux clean-mac clean-windows32 clean-windows64

#########################
#assemble for my distro
#########################
main:
	@echo -e making for `tput bold`$(DISTRO)`tput sgr0`... | awk '{sub(/-e /,""); print}' && $(MAKE) $(DISTRO) --no-print-directory

#########################
#assemble for all distros
#########################
deploy:
	@mkdir -p deploy
	@rm ./deploy/*.tar.gz -f 2>/dev/null
	@echo -e deploying for `tput bold`linux`tput sgr0`... | awk '{sub(/-e /,""); print}' && tar -zcvf deploy/lin.tar.gz release/lin
	@echo -e deploying for `tput bold`mac`tput sgr0`... | awk '{sub(/-e /,""); print}' && tar -zcvf deploy/mac.tar.gz release/mac
	@echo -e deploying for `tput bold`windows 32 bit`tput sgr0`... | awk '{sub(/-e /,""); print}' && tar -zcvf deploy/w32.tar.gz release/w32
	@echo -e deploying for `tput bold`windows 64 bit`tput sgr0`... | awk '{sub(/-e /,""); print}' && tar -zcvf deploy/w64.tar.gz release/w64

all:
	@echo -e making for `tput bold`linux`tput sgr0`... | awk '{sub(/-e /,""); print}' && $(MAKE) linux --no-print-directory
	@echo -e making for `tput bold`mac`tput sgr0`... | awk '{sub(/-e /,""); print}' && $(MAKE) mac --no-print-directory
	@echo -e making for `tput bold`windows 32 bit`tput sgr0`... | awk '{sub(/-e /,""); print}' && $(MAKE) windows32 --no-print-directory
	@echo -e making for `tput bold`windows 64 bit`tput sgr0`... | awk '{sub(/-e /,""); print}' && $(MAKE) windows64 --no-print-directory

#########################
#Assemble OS specific
#########################
linux:
	@echo -e '\t'Compiling object code... | awk '{sub(/-e /,""); print}'
	@$(MAKE) -C bin/binl all --no-print-directory | grep -vE "(Nothing to be done for|is up to date)"
	@echo -e '\t'Done Compiling | awk '{sub(/-e /,""); print}'
	@mkdir -p release/lin
	@echo -e '\t'Assembling executable... | awk '{sub(/-e /,""); print}'
	@$(MAKE) release/lin/run$(EXTL) --no-print-directory | grep -vE "(Nothing to be done for|is up to date)"
	@echo -e '\t'Done Assembling | awk '{sub(/-e /,""); print}'

mac:
	@echo -e '\t'Compiling object code... | awk '{sub(/-e /,""); print}'
	@$(MAKE) -C bin/binm all --no-print-directory | grep -vE "(Nothing to be done for|is up to date)"
	@echo -e '\t'Done Compiling | awk '{sub(/-e /,""); print}'
	@mkdir -p release/mac
	@echo -e '\t'Assembling executable... | awk '{sub(/-e /,""); print}'
	@$(MAKE) release/mac/run$(EXTM) --no-print-directory | grep -vE "(Nothing to be done for|is up to date)"
	@echo -e '\t'Done Assembling | awk '{sub(/-e /,""); print}'

windows32:
	@echo -e '\t'Compiling object code... | awk '{sub(/-e /,""); print}'
	@$(MAKE) -C bin/binw32 all --no-print-directory | grep -vE "(Nothing to be done for|is up to date)"
	@echo -e '\t'Done Compiling | awk '{sub(/-e /,""); print}'
	@mkdir -p release/w32
	@echo -e '\t'Assembling executable... | awk '{sub(/-e /,""); print}'
	@$(MAKE) release/w32/run$(EXTW32) --no-print-directory | grep -vE "(Nothing to be done for|is up to date)"
	@echo -e '\t'Done Assembling | awk '{sub(/-e /,""); print}'

windows64:
	@echo -e '\t'Compiling object code... | awk '{sub(/-e /,""); print}'
	@$(MAKE) -C bin/binw64 all --no-print-directory  | grep -vE "(Nothing to be done for|is up to date)"
	@echo -e '\t'Done Compiling | awk '{sub(/-e /,""); print}'
	@mkdir -p release/w64
	@echo -e '\t'Assembling executable... | awk '{sub(/-e /,""); print}'
	@$(MAKE) release/w64/run$(EXTW64) --no-print-directory | grep -vE "(Nothing to be done for|is up to date)"
	@echo -e '\t'Done Assembling | awk '{sub(/-e /,""); print}'

#########################
#Assemble executable
#########################
release/lin/run$(EXTL): bin/binl/*.o
	@echo -e '\t\t'Executable is being assembled... | awk '{sub(/-e /,""); print}'
	@rm -rf ./release/lin/* 2>/dev/null
	@$(CCL) $(CFLAGS) $(CFLAGSL) $^ -o $@ $(LIBSL)
	@cp bin/binl/*.dll release/lin/ 2>/dev/null || :
	@cp bin/*.dll release/lin/ 2>/dev/null || :
	
release/mac/run$(EXTM): bin/binm/*.o
	@echo -e '\t\t'Executable is being assembled... | awk '{sub(/-e /,""); print}'
	@rm -rf ./release/mac/* 2>/dev/null
	@$(CCL) $(CFLAGS) $(CFLAGSL) $^ -o $@ $(LIBSM)
	@cp bin/binm/*.dll release/mac/ 2>/dev/null || :
	@cp bin/*.dll release/mac/ 2>/dev/null || :
	
release/w32/run$(EXTW32): bin/binw32/*.o
	@echo -e '\t\t'Executable is being assembled... | awk '{sub(/-e /,""); print}'
	@rm -rf ./release/w32/* 2>/dev/null
	@$(CCW32) $(CFLAGS) $(CFLAGSW) $^ -o $@ $(LIBSW32)
	@cp bin/binw32/*.dll release/w32/ 2>/dev/null || :
	@cp bin/*.dll release/w32/ 2>/dev/null || :
	
release/w64/run$(EXTW64): bin/binw64/*.o
	@echo -e '\t\t'Executable is being assembled... | awk '{sub(/-e /,""); print}'
	@rm -rf ./release/w64/* 2>/dev/null
	@$(CCW64) $(CFLAGS) $(CFLAGSW) $^ -o $@ $(LIBSW64)
	@cp bin/binw64/*.dll release/w64/ 2>/dev/null || :
	@cp bin/*.dll release/w64/ 2>/dev/null || :

#########################
#Utility functions
#########################
help:
	@echo -e "Available build targets:\
\n\t`tput bold`Command;Description`tput sgr0`\
\n\tmain;Build for detected operating system\
\n\tall;Build for all operating systems\
\n\tlinux;Build for linux\
\n\tmac;Build for mac\
\n\twindows32;Build for Windows 32 bit\
\n\twindows64;Build for windows 64 bit\
\n\trelease/lin/run$(EXTL);Assemble linux executable\
\n\trelease/mac/run$(EXTM);Assemble mac executable\
\n\trelease/w32/run$(EXTW32);Assemble Windows 32 bit executable\
\n\trelease/w64/run$(EXTW64);Assemble Windows 64 bit executable\
\n\trun;Run for detected operating system\
\n\tcl;full clean for detected operating system\
\n\tclean;full clean for all operating systems\
\n\tclean-linux;full clean for linux\
\n\tclean-mac;full clean for mac\
\n\tclean-windows32;full clean for windows 32 bit\
\n\tclean-windows64;full clean for windows 64 bit\
\n\tfix;Replaces spaces with tabs in source files\
\n\thelp;Print availabe commands" | awk '{sub(/-e /,""); print}' | column -t -s ';'

run: release/lin/run$(EXTL)
	@cd release/lin && ./run$(EXTL)

cl:
	@echo cleaning `tput bold`$(DISTRO)`tput sgr0` shaders... | awk '{sub(/-e /,""); print}'
	@$(MAKE) clean-$(DISTRO) --no-print-directory | grep -vE "(Nothing to be done for|is up to date)"
	@echo Done Cleaning | awk '{sub(/-e /,""); print}'

clean:
	@echo Cleaning... | awk '{sub(/-e /,""); print}'
	@echo -e '\t'Cleaning `tput bold`linux`tput sgr0`... | awk '{sub(/-e /,""); print}'
	@$(MAKE) clean-linux --no-print-directory | grep -vE "(Nothing to be done for|is up to date)"
	@echo -e '\t'Done Cleaning | awk '{sub(/-e /,""); print}'
	@echo -e '\t'Cleaning `tput bold`mac`tput sgr0`...  | awk '{sub(/-e /,""); print}'
	@$(MAKE) clean-mac --no-print-directory | grep -vE "(Nothing to be done for|is up to date)"
	@echo -e '\t'Done Cleaning | awk '{sub(/-e /,""); print}'
	@echo -e '\t'Cleaning `tput bold`windows32`tput sgr0`...  | awk '{sub(/-e /,""); print}'
	@$(MAKE) clean-windows32 --no-print-directory | grep -vE "(Nothing to be done for|is up to date)"
	@echo -e '\t'Done Cleaning | awk '{sub(/-e /,""); print}'
	@echo -e '\t'Cleaning `tput bold`windows64`tput sgr0`... | awk '{sub(/-e /,""); print}'
	@$(MAKE) clean-windows64 --no-print-directory | grep -vE "(Nothing to be done for|is up to date)"
	@echo -e '\t'Done Cleaning | awk '{sub(/-e /,""); print}'
	@echo Done cleaning | awk '{sub(/-e /,""); print}'

clean-linux:
	@echo -e '\t\t'Cleaning object files in binl... | awk '{sub(/-e /,""); print}'
	@rm ./bin/binl/*.o -f 2>/dev/null || :
	@echo -e '\t\t'Cleaning files in release... | awk '{sub(/-e /,""); print}'
	@rm -rf ./release/lin/* -f 2>/dev/null || :

clean-mac:
	@echo -e '\t\t'Cleaning object files in binm... | awk '{sub(/-e /,""); print}'
	@rm ./bin/binm/*.o -f 2>/dev/null || :
	@echo -e '\t\t'Cleaning files in release... | awk '{sub(/-e /,""); print}'
	@rm -rf ./release/mac/* -f 2>/dev/null || :

clean-windows32:
	@echo -e '\t\t'Cleaning object files in binw32... | awk '{sub(/-e /,""); print}'
	@rm ./bin/binw32/*.o -f 2>/dev/null || :
	@echo -e '\t\t'Cleaning files in release... | awk '{sub(/-e /,""); print}'
	@rm -rf ./release/w32/* -f 2>/dev/null || :

clean-windows64:
	@echo -e '\t\t'Cleaning object files in binw64... | awk '{sub(/-e /,""); print}'
	@rm ./bin/binw64/*.o -f 2>/dev/null || :
	@echo -e '\t\t'Cleaning files in release... | awk '{sub(/-e /,""); print}'
	@rm -rf ./release/w64/* -f 2>/dev/null || :

fix:
	@echo Replacing spaces with tabs in source files | awk '{sub(/-e /,""); print}'
	@./scripts/fix.sh src
