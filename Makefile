NAME = webserv
CXX = c++
CXXFLAGS = -std=c++98 -Wall -Wextra -Werror -MMD -MP

SRCS = srcs/main.cpp

OBJS = $(SRCS:srcs/%.cpp=obj/%.o)
DEPS = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

obj/%.o: srcs/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -rf obj/

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

-include $(DEPS)
