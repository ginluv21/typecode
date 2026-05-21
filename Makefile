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

coverage:
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
