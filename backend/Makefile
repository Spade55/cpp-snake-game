# Makefile for Snake Game

CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2

# Windows-specific flags
ifeq ($(OS),Windows_NT)
    LDFLAGS = -static-libgcc -static-libstdc++
    TARGET_SNAKE = snake_game.exe
    TARGET_SCORE = score_tracker.exe
    TARGET_MENU = game_menu.exe
else
    TARGET_SNAKE = snake_game
    TARGET_SCORE = score_tracker
    TARGET_MENU = game_menu
endif

all: snake score_tracker menu

snake: snake_game.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET_SNAKE) snake_game.cpp $(LDFLAGS)

score_tracker: score_tracker.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET_SCORE) score_tracker.cpp $(LDFLAGS)

menu: game_menu.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET_MENU) game_menu.cpp $(LDFLAGS)

clean:
	rm -f $(TARGET_SNAKE) $(TARGET_SCORE) $(TARGET_MENU) *.o scores.txt

.PHONY: all clean

