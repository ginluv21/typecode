.PHONY: all run debug test coverage check-deps clean

BUILD_DIR = build
OBJ_DIR   = $(BUILD_DIR)/obj

SRC = src/main.c src/lesson.c src/lessons.c src/menu.c src/settings.c src/config.c src/stats.c src/ui.c src/heatmap.c
OBJ = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(SRC))

all: check-deps $(BUILD_DIR)/typecode

check-deps:
	@command -v gcc >/dev/null 2>&1 || { echo "Error: gcc not found. Install gcc."; exit 1; }
	@pkg-config --exists ncurses 2>/dev/null || \
	  find /usr /usr/local /opt/homebrew -name "ncurses.h" 2>/dev/null | grep -q . || \
	  { echo "Error: ncurses not found. Install: apt install libncurses-dev / pacman -S ncurses / dnf install ncurses-devel / brew install ncurses"; exit 1; }

$(BUILD_DIR)/typecode: $(OBJ)
	gcc -Wall -Wextra -std=c11 -I include -I tests $(OBJ) -o $@ -lncurses

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	gcc -Wall -Wextra -std=c11 -I include -I tests -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

run: all
	./$(BUILD_DIR)/typecode

debug: check-deps $(OBJ)
	gcc -Wall -Wextra -std=c11 -g -fsanitize=address,undefined -I include -I tests $(OBJ) -o $(BUILD_DIR)/typecode -lncurses

test: check-deps
	@mkdir -p $(BUILD_DIR)
	@for f in tests/test_*.c; do \
	  [ -f "$$f" ] || continue; \
	  gcc -Wall -Wextra -std=c11 -I include -I tests -o $(BUILD_DIR)/$$(basename $$f .c) \
	    $$f $$(ls src/*.c | grep -v main) -lncurses && \
	  $(BUILD_DIR)/$$(basename $$f .c) || exit 1; \
	done

coverage: check-deps
	@python3 -c "import gcovr" 2>/dev/null || \
	  { echo "Error: gcovr not found. Install: pip3 install gcovr"; exit 1; }
	@mkdir -p $(BUILD_DIR)
	@rm -f $(BUILD_DIR)/*.gcda $(BUILD_DIR)/*.gcno
	@for f in tests/test_*.c; do \
	  [ -f "$$f" ] || continue; \
	  gcc -Wall -Wextra -std=c11 -I include -I tests --coverage \
	    -o $(BUILD_DIR)/$$(basename $$f .c) $$f $$(ls src/*.c | grep -v main) -lncurses && \
	  ./$(BUILD_DIR)/$$(basename $$f .c) || exit 1; \
	done
	@python3 -m gcovr --object-directory $(BUILD_DIR) --filter src/settings.c --filter src/stats.c --filter src/lesson.c --filter src/lessons.c --print-summary --fail-under-line 60
	@rm -f $(BUILD_DIR)/*.gcda $(BUILD_DIR)/*.gcno

clean:
	rm -rf $(BUILD_DIR)
