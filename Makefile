OPT:=-O3 -pipe 
CXXFLAGS:=`pkg-config opencv --cflags` -I. -Iinclude -g -MMD -Wall $(OPT) -flto
LDFLAGS:=-Wl,--no-as-needed -lavcodec -lavformat -lavutil -lswscale `pkg-config opencv --libs` -lm -lexpat -ltesseract -flto

PROGS:=extract-features-ref make-ref predict-bow shot-boundary-detector view-shot-boundaries subshot-detector temporal-median tess-ocr train-bow haar-detector view-ascii hog-view extract-hog

############################### under the hood ######

# main targets: build executables, make bundles and clean
all: $(addprefix bin/,$(PROGS))
bundle: $(addprefix bin/,$(PROGS:=.bundle))
clean:
	rm -rf .compile/* bin/*

MAKEFLAGS+=-s -j$(shell grep ^processor /proc/cpuinfo |wc -l)

# disable implicit rules
.SUFFIXES:

# bundle executable with libraries
bin/%.bundle: bin/%
	@echo "  BUNDLE $@"
	@mkdir -p bin
	scripts/make-bundle $< $@

# rule for creating executables including extra objects
bin/%: .compile/%.o $(OBJ)
	@echo "  LINK $@"
	@mkdir -p bin
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@

# rule for compiling c++ sources from subdirectories of src/
.SECONDEXPANSION:
.compile/%.o: $$(shell find src -name %.cc)
	@echo "  COMPILE $@"
	@mkdir -p .compile
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

# do not delete objects
.PRECIOUS: .compile/%.o

# include dependencies generated with "gcc -MMD"
-include $(shell find . -name \*.d)
