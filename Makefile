CXX = g++
CXXFLAGS = -std=c++17 -O2

# Source files
SRC = main.cpp glad/glad.c
OUT = app

# Include the 'glad' folder (where glad.h and gl.h are)
INCLUDES = -Iglad

# Link GLFW, OpenGL, and dl
LIBS = -lglfw -ldl -lGL

# Default target
all: $(OUT)

# Build rule
$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(OUT) $(SRC) $(INCLUDES) $(LIBS)

# Run the app
run: $(OUT)
	./$(OUT)

# Clean build artifacts
clean:
	rm -f $(OUT) *.o
