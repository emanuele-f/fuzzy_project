# Makefile to test fuzzy library units
# depends on /src headers and on /build/libfuzzy.a

SHELL=/bin/bash
ROOT_FOLDER=$(shell readlink -f ../../)
TESTS_FOLDER=$(shell readlink -f .)
UTILS_FOLDER=$(ROOT_FOLDER)/utils
BUILD_FOLDER=$(ROOT_FOLDER)/build
SRC_FOLDER=$(ROOT_FOLDER)/src
DEP_FOLDER=$(SRC_FOLDER)/deps
CFLAGS = -Wall -g  -I $(SRC_FOLDER) -I$(DEP_FOLDER)/tmx/src \
	-fprofile-arcs -ftest-coverage
LDFLAGS = -L $(BUILD_FOLDER) -L$(BUILD_FOLDER)/tmx
LDLIBS = -lgcov -lfuzzy -lz -lxml2 -ltmx

.PHONY: default all clean
default: all

# All test outputs here
TESTS = network tilesystem
TEST_TARGETS = $(addsuffix -test, $(addprefix $(BUILD_FOLDER)/, $(TESTS)))
TESTS_TRACE=$(BUILD_FOLDER)/malloc_trace
VALGRIND_ERROR=77
VALGRIND_TRACE=$(BUILD_FOLDER)/valgrind_out
GCOV_TRACE=$(BUILD_FOLDER)/gcov_out

# Matches any test in build folder to be built
$(BUILD_FOLDER)/%-test: %-test.c $(BUILD_FOLDER)/fakeallegro.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(BUILD_FOLDER)/fakeallegro.o -o $@ $< $(LDLIBS)

$(BUILD_FOLDER)/fakeallegro.o: fakeallegro.c
	$(CC) $(CFLAGS) -c -o $@ $<

all:
	make clean
	make $(TEST_TARGETS)
	@echo -e "\n=== Running mytests ==="
	@for test in $(TESTS); \
	do \
		function test { echo -en "- Testing\033[1;37m $$1\033[0m module"; }; \
		function passed { echo -en "\033[1;32m$$1\033[0m"; }; \
		function failed { echo -en "\033[1;31m$$1\033[0m"; }; \
		echo -en "\t$$(test $$test)..."; \
		MALLOC_TRACE=$(TESTS_TRACE) valgrind -q --log-file=$(VALGRIND_TRACE) --error-exitcode=$(VALGRIND_ERROR) $(BUILD_FOLDER)/$$test-test; \
		err=$$?; \
		[ $$err -eq $(VALGRIND_ERROR) ] && echo $$(failed "valgrind error") && cat $(VALGRIND_TRACE) && exit $(VALGRIND_ERROR); \
		[ $$err -ne 0 ] && echo $$(failed failed) && exit $$err; \
		mtrace $(BUILD_FOLDER)/$$test-test $(TESTS_TRACE) >/dev/null;\
		err=$$?; \
		[ $$err -ne 0 ] && echo $$(failed "mtrace error") && cat $(TESTS_TRACE) && exit $$err; \
		echo $$(passed ok); \
	done
	@echo -e "=== Tests passed ===\n"

	@echo "=== Tests coverage summary ==="
	@cd $(BUILD_FOLDER) && gcov -n $(shell ls $(SRC_FOLDER)) 1> $(GCOV_TRACE) 2>/dev/null
	@$(UTILS_FOLDER)/gcovfmat.sh < $(GCOV_TRACE)

clean:
	rm -f $(GCOV_TRACE)
	rm -f $(TESTS_TRACE) $(VALGRIND_TRACE)
	rm -f $(TEST_TARGETS)
	rm -f *.gcno *.gcda
