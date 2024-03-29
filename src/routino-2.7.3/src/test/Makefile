# Test cases Makefile
#
# Part of the Routino routing software.
#
# This file Copyright 2011-2014 Andrew M. Bishop
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# All configuration is in the top-level Makefile.conf

include ../../Makefile.conf

# executables

ROUTINO_EXE=../planetsplitter ../planetsplitter-slim \
            ../router ../router-slim \
            ../filedumper ../filedumper-slim

EXE=is-fast-math

# Compilation targets

O=$(notdir $(wildcard *.osm))
S=$(foreach f,$(O),$(addsuffix .sh,$(basename $f)))

########

all :

########

test : $(ROUTINO_EXE) $(EXE)
	@status=true ;\
	echo ""; \
	./is-fast-math message; \
	for script in $(S); do \
	   echo "" ;\
	   echo "Testing: $$script (non-slim, no pruning) ... " ;\
	   if ./$$script fat; then echo "... passed"; else echo "... FAILED"; status=false; fi ;\
	done ;\
	for script in $(S); do \
	   echo "" ;\
	   echo "Testing: $$script (slim, no pruning) ... " ;\
	   if ./$$script slim; then echo "... passed"; else echo "... FAILED"; status=false; fi ;\
	done ;\
	echo "" ;\
	if $$status; then echo "Success: all tests passed"; else echo "Warning: Some tests FAILED"; fi ;\
	$$status || exit 1 ;\
	echo "" ;\
	echo "Comparing: slim and non-slim results ... " ;\
	if diff -q -r slim fat; then echo "... matched"; else echo "... match FAILED"; status=false; fi ;\
	echo "" ;\
	if $$status; then echo "Success: slim and non-slim results match"; else echo "Warning: slim and non-slim results are different - FAILED"; fi ;\
	for script in $(S); do \
	   echo "" ;\
	   echo "Testing: $$script (non-slim, pruning) ... " ;\
	   if ./$$script fat prune; then echo "... passed"; else echo "... FAILED"; status=false; fi ;\
	done ;\
	for script in $(S); do \
	   echo "" ;\
	   echo "Testing: $$script (slim, pruning) ... " ;\
	   if ./$$script slim prune; then echo "... passed"; else echo "... FAILED"; status=false; fi ;\
	done ;\
	echo "" ;\
	if $$status; then echo "Success: all tests passed"; else echo "Warning: Some tests FAILED"; fi ;\
	$$status || exit 1 ;\
	echo "" ;\
	echo "Comparing: slim and non-slim results ... " ;\
	if diff -q -r slim-pruned fat-pruned; then echo "... matched"; else echo "... match FAILED"; status=false; fi ;\
	echo "" ;\
	if $$status; then echo "Success: slim and non-slim results match"; else echo "Warning: slim and non-slim results are different - FAILED"; fi ;\
	$$status

########

$(ROUTINO_EXE) ::
	cd .. && $(MAKE) $(notdir $@)

is-fast-math : is-fast-math.o
	$(LD) $< -o $@ $(LDFLAGS)

is-fast-math.o : is-fast-math.c
	$(CC) -c $(CFLAGS) $< -o $@

########

install:

########

clean:
	rm -rf fat
	rm -rf slim
	rm -rf fat-pruned
	rm -rf slim-pruned
	rm -f *.log
	rm -f *~
	rm -f *.o
	rm -f core
	rm -f *.gcda *.gcno *.gcov gmon.out

########

distclean: clean
	rm -f is-fast-math

########

.PHONY:: all test install clean distclean
