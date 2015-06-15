# Makefile to test fuzzy library units
# depends on /src headers and on /build/libfuzzy.a

ROOT_FOLDER=$(shell readlink -f ../../)
TESTS_FOLDER=$(shell readlink -f .)
UTILS_FOLDER=$(ROOT_FOLDER)/utils
BUILD_FOLDER=$(ROOT_FOLDER)/build
SRC_FOLDER=$(ROOT_FOLDER)/src
CFLAGS = -Wall -g  -I ../ -fprofile-arcs -ftest-coverage
LDFLAGS = -L $(BUILD_FOLDER)
LDLIBS = -lgcov -lfuzzy

.PHONY: default all clean
default: all

# All test outputs here
TESTS = network
TESTS_TRACE=./malloc_trace
VALGRIND_ERROR=77
VALGRIND_TRACE=./valgrind_out
GCOV_TRACE=$(TESTS_FOLDER)/gcov_out

all:
	make clean
	make $(TESTS)
	@echo -e "\n=== Running mytests ==="
	@for test in $(TESTS); \
	do \
		function test { echo -en "- Testing\033[1;37m $$1\033[0m module"; }; \
		function passed { echo -en "\033[1;32m$$1\033[0m"; }; \
		function failed { echo -en "\033[1;31m$$1\033[0m"; }; \
		echo -en "\t$$(test $$test)..."; \
		MALLOC_TRACE=$(TESTS_TRACE) valgrind -q --log-file=$(VALGRIND_TRACE) --error-exitcode=$(VALGRIND_ERROR) ./$$test; \
		err=$$?; \
		[ $$err -eq $(VALGRIND_ERROR) ] && echo $$(failed "valgrind error") && cat $(VALGRIND_TRACE) && exit $(VALGRIND_ERROR); \
		[ $$err -ne 0 ] && echo $$(failed failed) && exit $$err; \
		mtrace ./$$test $(TESTS_TRACE) >/dev/null;\
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
	rm -f $(TESTS)
	rm -f *.gcno *.gcda
