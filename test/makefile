cc_flags = -Wall -std=c++17 -g -O3

runner = test_runner
shell = test.sh
test_out = test.out

srcdir = ../src
jarl_source_files = $(shell ls $(srcdir)/*.{cpp,h})

tests = $(shell ls *.jarl)
test_targets = $(subst .jarl,.out,$(tests))

.PHONY: all full clean

all: $(test_targets)
	@./$(shell) new_tests

full:
	@touch *.jarl
	@$(MAKE) --no-print-directory

%.out: %.jarl $(runner)
	@./$(shell) $@

$(runner): $(runner).cpp libjarl.a
	$(CXX) $(cc_flags) -L./ -o $@ $< -ljarl

$(runner).o: $(runner).cpp
	$(CXX) $(cc_flags) -c -o $@ $<

libjarl.a: $(jarl_source_files)
	cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
	$(MAKE) --no-print-directory -f Makefile

clean:
	rm $(test_runner) $(test_runner).o
	$(MAKE) --no-print-directory -f Makefile clean

