.PHONY: all run debug test coverage check-deps clean

check-deps:
	@command -v gcc >/dev/null 2>&1 || { echo "Error: gcc not found. Install gcc."; exit 1; }
	@pkg-config --exists ncurses 2>/dev/null || \
	  find /usr /usr/local /opt/homebrew -name "ncurses.h" 2>/dev/null | grep -q . || \
	  { echo "Error: ncurses not found. Install: apt install libncurses-dev / pacman -S ncurses / dnf install ncurses-devel / brew install ncurses"; exit 1; }

all: check-deps
	gcc -Wall -Wextra -std=c11 -I include -I tests src/*.c -o typecode -lncurses

run: all
	./typecode

debug: check-deps
	gcc -Wall -Wextra -std=c11 -g -fsanitize=address,undefined -I include -I tests src/*.c -o typecode -lncurses

test: check-deps
	@mkdir -p build
	@for f in tests/test_*.c; do \
	  [ -f "$$f" ] || continue; \
	  gcc -Wall -Wextra -std=c11 -I include -I tests -o build/$$(basename $$f .c) \
	    $$f $$(ls src/*.c | grep -v main) -lncurses && \
	  build/$$(basename $$f .c) || exit 1; \
	done

coverage: check-deps
	@python3 -c "import gcovr" 2>/dev/null || \
	  { echo "Error: gcovr not found. Install: pip3 install gcovr"; exit 1; }
	@mkdir -p build
	@rm -f build/*.gcda build/*.gcno
	@for f in tests/test_*.c; do \
	  [ -f "$$f" ] || continue; \
	  gcc -Wall -Wextra -std=c11 -I include -I tests --coverage \
	    -o build/$$(basename $$f .c) $$f $$(ls src/*.c | grep -v main) -lncurses && \
	  ./build/$$(basename $$f .c) || exit 1; \
	done
	@python3 -m gcovr --object-directory build --filter src/settings.c --filter src/stats.c --filter src/lesson.c --print-summary --fail-under-line 60
	@rm -f build/*.gcda build/*.gcno

clean:
	rm -rf build typecode
