# SymSpell C99 - Simple Makefile
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -Iinclude

.PHONY: all test benchmark clean help

all: test_symspell benchmark_symspell

test_symspell: test/test_symspell.c src/symspell.c
	$(CC) $(CFLAGS) $^ -o $@

benchmark_symspell: test/benchmark_symspell.c src/symspell.c
	$(CC) $(CFLAGS) $^ -o $@

test: test_symspell
	./test_symspell dictionaries/dictionary.txt

benchmark: benchmark_symspell
	./benchmark_symspell dictionaries/dictionary.txt test/data/symspell/misspellings/misspell-codespell.txt

clean:
	rm -f test_symspell benchmark_symspell

help:
	@echo "SymSpell C99 Build Targets:"
	@echo "  make          - Build test and benchmark programs"
	@echo "  make test     - Build and run tests"
	@echo "  make benchmark - Build and run benchmark"
	@echo "  make all      - Same as 'make'"
	@echo "  make clean    - Remove built programs"
	@echo "  make help     - Show this help"
