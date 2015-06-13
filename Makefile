
# Globals
BUILD_FOLDER=$(shell readlink -f ./build)
SRC_FOLDER=$(shell readlink -f ./src)
DEP_FOLDER=$(SRC_FOLDER)/deps
TOOLS_FOLDER=$(SRC_FOLDER)/tools

# Flags and libs
DEP_FLAGS=-I $(DEP_FOLDER)/allegro_tiled/include
CFLAGS=-Wall
STD_LIBS=-pthread -lrt -lm -lz -lglib-2.0 -lxml2
ALLEGRO_LIBS=$(shell pkg-config --libs allegro-5.0 allegro_image-5.0 allegro_primitives-5.0 allegro_font-5.0)

# Dependencies targets
DEP_ALLEGRO_TILED=$(BUILD_FOLDER)/allegro_tiled/liballegro_tiled.a

main: $(SRC_FOLDER)/main.c $(DEP_ALLEGRO_TILED) $(SRC_FOLDER)/global.h
	echo $(CFLAGS)
	gcc $(CFLAGS) $(DEP_FLAGS) $(STD_LIBS) $(ALLEGRO_LIBS) -o main $^

$(DEP_ALLEGRO_TILED):
	mkdir -p $(BUILD_FOLDER)/allegro_tiled
	cd $(BUILD_FOLDER)/allegro_tiled && cmake $(DEP_FOLDER)/allegro_tiled && make

.PHONY: debug init tools clean cleanall

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

tools: tiled

tiled:
	cp -r $(TOOLS_FOLDER)/tiled-0.12.3 $(BUILD_FOLDER)/tiled
	cd $(BUILD_FOLDER)/tiled && qmake && make

clean:
	rm -rf $(BUILD_FOLDER)/*
	rm -f main

cleanall: clean
	rm -rf $(DEP_FOLDER)/*
	rm -rf $(TOOLS_FOLDER)/*
