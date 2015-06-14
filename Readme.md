#Fuzzy Project

Allegro 5 sperimentation, server-client comunication, 2D tiled world...with
a possible strategic game future in mind.

See src/network module for a minimal implementation of a TCP endianess/bitness
independent network library.

See src/fuzzy for a set of utility functions.

Dependencies
------------
- allegro5
- tmx: cmake zlib libxml2
- libuuid

Optional deps
-------------
- tiled - a tiled map editor: qmake qt4

Make rules
----------

- make init: pulls in the dependencies and tools
- make: build the game
- make debug: enables debug info. requires a 'clean' to rebuild
- make tools: builds external tools, like the Tiled map editor
- make clean: removes any built binary
- make cleanall: removes any built binary, any dependency and any tool
