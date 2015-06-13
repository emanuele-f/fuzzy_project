
# Globals
BUILD_FOLDER=$(shell readlink -f ./build)
SRC_FOLDER=$(shell readlink -f ./src)
DEP_FOLDER=$(SRC_FOLDER)/deps
TOOLS_FOLDER=$(SRC_FOLDER)/tools
LIB_FOLDER=$(BUILD_FOLDER)

# Flags and libs
DEP_FLAGS=-I $(DEP_FOLDER)/allegro_tiled/include
CFLAGS=-Wall -L$(LIB_FOLDER) -L$(BUILD_FOLDER)/allegro_tiled
STD_LIBS=-pthread -lrt -lm -lz -lglib-2.0 -lxml2 -luuid
MY_LIBS=$(shell pkg-config --libs allegro-5.0 allegro_image-5.0\
  allegro_primitives-5.0 allegro_font-5.0) -lallegro_tiled -lfuzzy

default: main

# OBJ targets
OBJ_TARGETS_ = fuzzy.o network.o protocol.o server.o

$(BUILD_FOLDER)/main.o: $(SRC_FOLDER)/main.c $(SRC_FOLDER)/fuzzy.h
	$(CC) $(CFLAGS) $(DEP_FLAGS) -c -o $@ $<

OBJ_TARGETS = $(addprefix $(BUILD_FOLDER)/, $(OBJ_TARGETS_))
$(BUILD_FOLDER)/%.o: $(SRC_FOLDER)/%.c $(SRC_FOLDER)/%.h $(SRC_FOLDER)/fuzzy.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Targets
LIB_ALLEGRO_TILED=$(BUILD_FOLDER)/allegro_tiled/liballegro_tiled.a
LIB_FUZZY=$(BUILD_FOLDER)/libfuzzy.a

main: $(LIB_ALLEGRO_TILED) $(LIB_FUZZY) $(BUILD_FOLDER)/main.o
	$(CC) $(CFLAGS) $(STD_LIBS) -o main $(BUILD_FOLDER)/main.o $(MY_LIBS)

$(LIB_FUZZY): $(OBJ_TARGETS)
	rm -f $(BUILD_FOLDER)/libfuzzy.a
	make -e $(OBJ_TARGETS)
	ar -r $(BUILD_FOLDER)/libfuzzy.a $(OBJ_TARGETS)

$(LIB_ALLEGRO_TILED):
	mkdir -p $(BUILD_FOLDER)/allegro_tiled
	cd $(BUILD_FOLDER)/allegro_tiled && cmake $(DEP_FOLDER)/allegro_tiled && make

tiles-editor:
	cp -r $(TOOLS_FOLDER)/tiled-0.12.3 $(BUILD_FOLDER)/tiled
	mkdir -p $(BUILD_FOLDER)/tiled/lib
	ln -sf $(BUILD_FOLDER)/tiled/lib/libtiled.so.1.0.0 $(BUILD_FOLDER)/tiled/lib/libtiled.so
	cd $(BUILD_FOLDER)/tiled && qmake && make
	mkdir -p $(BUILD_FOLDER)/tiled/lib/tls/i686/sse2/cmov
	cp $(BUILD_FOLDER)/tiled/lib/libtiled.so.1.0.0 $(BUILD_FOLDER)/tiled/lib/tls/i686/sse2/cmov/libtiled.so.1
	ln -sf $(BUILD_FOLDER)/tiled/bin/tiled ./tiles-editor

# PHONY targets

.PHONY: default debug init tools clean cleanall

debug: export CFLAGS += -g -DDEBUG
debug:
	@echo "'make clean' before changing build type!"
	make -e main

init:
	@echo Pulling dependencies...
	mkdir -p $(DEP_FOLDER)

	@echo -e \tAllegro tiled...
	-git clone https://github.com/dradtke/allegro_tiled $(DEP_FOLDER)/allegro_tiled

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
