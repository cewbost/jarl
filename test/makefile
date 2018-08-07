cc_flags = -Wall -std=c++17 -g -O3

test_bin = test_runner
test_runner = test_runner.sh
test_out = test.out

srcdir = ../src
jarl_source_files = $(shell ls $(srcdir)/*.{cpp,h})

tests = $(shell ls *.jarl)
test_targets = $(subst .jarl,.out,$(tests))

.PHONY: all full clean

all: $(test_targets)
	@./$(test_runner) new_tests

full:
	@touch *.jarl
	@$(MAKE) --no-print-directory

%.out: %.jarl $(test_bin) $(test_runner)
	@./$(test_runner) $@ $(valgrind)

$(test_bin): $(test_bin).cpp libjarl.a
	$(CXX) $(cc_flags) -L./ -o $@ $< -ljarl

$(test_bin).o: $(test_bin).cpp
	$(CXX) $(cc_flags) -c -o $@ $<

libjarl.a: $(jarl_source_files)
	cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
	$(MAKE) --no-print-directory -f Makefile

clean:
	rm $(test_bin)
	$(MAKE) --no-print-directory -f Makefile clean
