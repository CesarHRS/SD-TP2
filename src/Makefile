CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread
SRC_DIR = src
BUILD_DIR = build
TARGET = peer
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))

all: $(BUILD_DIR)/$(TARGET)
$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	@echo "Linkando executável..."
	$(CXX) $(CXXFLAGS) -o $@ $^
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compilando $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

clean:
	@echo "Limpando diretório de build..."
	@rm -rf $(BUILD_DIR)

test: all
	@echo "Executando testes..."
	@bash src/run_test.sh

.PHONY: all clean test
