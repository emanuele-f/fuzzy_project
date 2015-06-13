DEPS
----

- cmake
- allegro5
- allegro_tiled: xml2 zlib glib2.0

OPTIONAL TOOLS
--------------

- tiled: qmake qt4


RULES
-----

- make init: pulls in the dependencies and tools
- make: build the game
- make debug: enables debug info. requires a 'clean' to rebuild
- make clean: removes any built binary
- make cleanall: removes any built binary, any dependency and any tool
