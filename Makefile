OPT:=-O3
CXXFLAGS:=`pkg-config opencv --cflags` -I. -Iinclude -g -MMD -Wall $(OPT) #-flto
LDFLAGS:=-Wl,--as-needed -lavcodec -lavformat -lavutil -lswscale `pkg-config opencv --libs` -lm -lexpat -ltesseract -flto -lcurses -lfann
CXX=clang++

PROGS:=extract-features-ref make-ref predict-bow shot-boundary-detector view-shot-boundaries subshot-detector temporal-median tess-ocr train-bow haar-detector view-ascii hog-view extract-hog cumulative-gradients subshot-from-template extract-hog-subshot track-faces-in-shots get-hog-training-data detect-from-hog get-hog-extra-negatives classify-hog-subshot detector-performance get-hog-fddb tracker-from-detections merge-detections simple-detector view-shot-clustering play-video shot-type shot-roles view-detections

############################### under the hood ######

# main targets: build executables, make bundles and clean
all: $(addprefix bin/,$(PROGS))
bundle: $(addprefix bin/,$(PROGS:=.bundle))
bundle-dir: $(addprefix bin-with-libs/,$(PROGS))

clean:
	rm -rf .compile/* bin/* bin-with-libs

MAKEFLAGS+=-s -j$(shell grep ^processor /proc/cpuinfo |wc -l)
GIT_VERSION:=$(shell git describe --abbrev=6 --dirty --always)
CXXFLAGS+="-DVERSION=\"$(GIT_VERSION), $(shell date)\""

# disable implicit rules
.SUFFIXES:

# bundle executable with libraries
bin/%.bundle: bin/%.strip
	@echo "  BUNDLE $@"
	@mkdir -p bin
	scripts/make-bundle $< $@

bin-with-libs/%: bin/%
	@echo "  BUNDLE-DIR $@"
	scripts/make-bundle-dir $< bin-with-libs

# strip executable
bin/%.strip: bin/%
	cp $< $@
	#strip $< -o $@

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
