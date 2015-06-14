# Globals
BUILD_FOLDER=$(shell readlink -f ./build)
SRC_FOLDER=$(shell readlink -f ./src)
DEP_FOLDER=$(SRC_FOLDER)/deps
TOOLS_FOLDER=$(SRC_FOLDER)/tools
LIB_FOLDER=$(BUILD_FOLDER)

# Flags and libs
CFLAGS=-Wall -I$(DEP_FOLDER)/tmx/src -L$(LIB_FOLDER) -L$(BUILD_FOLDER)/tmx
STD_LIBS=-pthread -lrt -lm -lz -lglib-2.0 -lxml2 -luuid
MY_LIBS=$(shell pkg-config --libs allegro-5.0 allegro_image-5.0\
  allegro_primitives-5.0 allegro_font-5.0) -ltmx -lfuzzy

default: main

# OBJ targets
OBJ_TARGETS_ = tiles.o fuzzy.o network.o protocol.o server.o

$(BUILD_FOLDER)/main.o: $(SRC_FOLDER)/main.c $(SRC_FOLDER)/fuzzy.h
	$(CC) $(CFLAGS) -c -o $@ $<

OBJ_TARGETS = $(addprefix $(BUILD_FOLDER)/, $(OBJ_TARGETS_))
$(BUILD_FOLDER)/%.o: $(SRC_FOLDER)/%.c $(SRC_FOLDER)/%.h $(SRC_FOLDER)/fuzzy.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Targets
LIB_FUZZY=$(BUILD_FOLDER)/libfuzzy.a
LIB_TMX=$(BUILD_FOLDER)/tmx/libtmx.a

main: $(LIB_TMX) $(LIB_FUZZY) $(BUILD_FOLDER)/main.o
	$(CC) $(CFLAGS) $(STD_LIBS) -o main $(BUILD_FOLDER)/main.o $(MY_LIBS)

$(LIB_FUZZY): $(OBJ_TARGETS)
	rm -f $(BUILD_FOLDER)/libfuzzy.a
	make -e $(OBJ_TARGETS)
	ar -r $(BUILD_FOLDER)/libfuzzy.a $(OBJ_TARGETS)

$(LIB_TMX):
	mkdir -p $(BUILD_FOLDER)/tmx
	cd $(BUILD_FOLDER)/tmx && cmake -DWANT_JSON=off $(DEP_FOLDER)/tmx && make

tiles-editor:
	cp -r $(TOOLS_FOLDER)/tiled-0.12.3 $(BUILD_FOLDER)/tiled
	mkdir -p $(BUILD_FOLDER)/tiled/lib
	/bin/ln -sf $(BUILD_FOLDER)/tiled/lib/libtiled.so.1.0.0 $(BUILD_FOLDER)/tiled/lib/libtiled.so
	cd $(BUILD_FOLDER)/tiled && qmake && make
	mkdir -p $(BUILD_FOLDER)/tiled/lib/tls/i686/sse2/cmov
	cp $(BUILD_FOLDER)/tiled/lib/libtiled.so.1.0.0 $(BUILD_FOLDER)/tiled/lib/tls/i686/sse2/cmov/libtiled.so.1
	/bin/ln -sf $(BUILD_FOLDER)/tiled/bin/tiled ./tiles-editor

# PHONY targets

.PHONY: default debug init tools clean cleanall

debug: export CFLAGS += -g -DDEBUG
debug:
	@echo "'make clean' before changing build type!"
	make -e main

init:
	@echo Pulling dependencies...
	mkdir -p $(DEP_FOLDER)

	@echo -e \tTMX map library...
	-git clone https://github.com/baylej/tmx $(DEP_FOLDER)/tmx

	@echo Pulling tools...
	mkdir -p $(TOOLS_FOLDER)

	@echo -e \tTiled...
	wget -O $(TOOLS_FOLDER)/tiled-0.12.3.tar.gz https://github.com/bjorn/tiled/archive/v0.12.3/tiled-0.12.3.tar.gz
	tar -xzf $(TOOLS_FOLDER)/tiled-0.12.3.tar.gz -C $(TOOLS_FOLDER)

tools: tiles-editor

clean:
	rm -rf $(BUILD_FOLDER)/*
	rm -f main
	rm -f tiles-editor

cleanall: clean
	rm -rf $(DEP_FOLDER)/*
	rm -rf $(TOOLS_FOLDER)/*
