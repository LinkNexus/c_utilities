CC = cc
CFLAGS = -g -Wall -Wextra -std=c23 -I. -MMD -MP -fsanitize=address -fsanitize=undefined

BUILD_DIR = build
TEST_DIR = tests

TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%,$(TEST_SRCS))

$(BUILD_DIR)/%: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

-include $(TEST_BINS:=.d)

.PHONY: all clean test

all: test

test: $(TEST_BINS)

$(BUILD_DIR)/%: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: test
	@for test in $(TEST_BINS); do \
		echo "Running $$test"; \
		./$$test || exit 1; \
	done

clean:
	rm -rf $(BUILD_DIR)
