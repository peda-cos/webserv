NAME = webserv
COMPILER = c++
VERSION = -std=c++98
DEPS_HEADERS = -MMD -MP
SYNTAX = -Wall -Wextra -Werror
# TODO: Validar se pode? - E a mesma coisa que willcard para sources ?
INCLUDES = $(addprefix -I , $(shell find includes -type d)) 
# ---------------------------------------------------------
FLAGS = $(VERSION) $(SYNTAX) $(DEPS_HEADERS) $(INCLUDES)

LEXER_SRCS = srcs/config/ConfigLexer.cpp
UTILS_SRCS = srcs/utils/ConfigUtils.cpp srcs/utils/ParsingUtils.cpp

SRCS = srcs/main.cpp $(LEXER_SRCS) $(UTILS_SRCS)

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
