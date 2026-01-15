CC = g++
SFLAGS = -Wall -Wextra -Wpedantic -std=c++20
OFLAGS = -O3 -march=native -flto=auto -DNDEBUG
LFLAGS = -static
SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)
BIN = chessnut_mingw

$(BIN): $(OBJ)
	$(CC) $(OBJ) $(LFLAGS) -o $@

$(OBJ): %.o: %.cpp
	$(CC) $(SFLAGS) $(OFLAGS) -c $<

clean:
	@rm -f $(OBJ)
