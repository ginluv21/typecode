.PHONY: all run debug test clean

all:
	gcc -Wall -Wextra -std=c11 -I include -I tests src/*.c -o typecode -lncurses

run: all
	./typecode

debug:
	gcc -Wall -Wextra -std=c11 -g -fsanitize=address,undefined -I include -I tests src/*.c -o typecode -lncurses

test:
	@mkdir -p build
	@for f in tests/test_*.c; do \
	  [ -f "$$f" ] || continue; \
	  gcc -Wall -Wextra -std=c11 -I include -I tests -o build/$$(basename $$f .c) \
	    $$f $$(ls src/*.c | grep -v main) -lncurses && \
	  build/$$(basename $$f .c) || exit 1; \
	done

clean:
	rm -rf build typecode
