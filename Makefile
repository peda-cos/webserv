# Makefile for webserv - C++98 webserver
NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I./includes

# Source and object files
SRCS = $(wildcard src/core/*.cpp) $(wildcard src/http/*.cpp) src/main.cpp
OBJS = $(patsubst src/%.cpp,obj/%.o,$(SRCS))

# Directories for objects
OBJ_DIRS = obj obj/core obj/http

all: $(OBJ_DIRS) $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

obj/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIRS):
	mkdir -p $@

clean:
	rm -rf obj/

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
