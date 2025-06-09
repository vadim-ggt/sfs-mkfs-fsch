# Компилятор и флаги
CC = gcc
CFLAGS = -std=c11 -Wall -pedantic -Wextra -Werror -D_FILE_OFFSET_BITS=64 -Iinclude
LDFLAGS = 
DEBUG_FLAGS = -g -O0
RELEASE_FLAGS = -O2

# Директории
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Исходные файлы
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
MKFS_SRC = $(SRC_DIR)/mkfs_sfs.c
FSCH_SRC = $(SRC_DIR)/fsch.c  # Новый файл для утилиты fsch
OTHER_SRCS = $(filter-out $(MKFS_SRC) $(FSCH_SRC), $(SRC_FILES))
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(OTHER_SRCS))

# Цели
MKFS_TARGET = $(BIN_DIR)/mkfs.sfs
FSCH_TARGET = $(BIN_DIR)/fsch  # Исполнимый файл для fsch

.PHONY: all clean debug release install

# Режим по умолчанию
all: release

# Отладочная сборка
debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(MKFS_TARGET) $(FSCH_TARGET)

# Релизная сборка
release: CFLAGS += $(RELEASE_FLAGS)
release: $(MKFS_TARGET) $(FSCH_TARGET)

# Основная цель для mkfs.sfs
$(MKFS_TARGET): $(MKFS_SRC) $(OBJ_FILES)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Цель для fsch
$(FSCH_TARGET): $(FSCH_SRC) $(OBJ_FILES)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Правило для объектных файлов
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# Создание директорий
$(BIN_DIR) $(BUILD_DIR):
	@mkdir -p $@

# Очистка
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Установка
install: $(MKFS_TARGET) $(FSCH_TARGET)
	cp $(MKFS_TARGET) /usr/local/bin/
	cp $(FSCH_TARGET) /usr/local/bin/
