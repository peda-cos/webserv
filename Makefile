NAME = webserv
COMPILER = c++
VERSION = -std=c++98
INCLUDES = -I includes
DEPS_HEADERS = -MMD -MP
SYNTAX = -Wall -Wextra -Werror
FLAGS = $(VERSION) $(SYNTAX) $(DEPS_HEADERS) $(INCLUDES)

SRCS = srcs/main.cpp

OBJS = $(SRCS:srcs/%.cpp=objs/%.o)
DEPS = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(COMPILER) $(FLAGS) $(OBJS) -o $(NAME)

objs/%.o: srcs/%.cpp
	@mkdir -p $(dir $@)
	$(COMPILER) $(FLAGS) -c $< -o $@

clean:
	@rm -rf objs/

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

-include $(DEPS)
