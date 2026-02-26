NAME = webserv
CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98 -I./includes
SRCS = src/main.cpp \
       src/core/Connection.cpp \
       src/core/EventLoop.cpp \
       src/core/Listener.cpp \
       src/http/HttpParser.cpp \
       src/config/Token.cpp \
       src/config/Lexer.cpp \
       src/config/ConfigNode.cpp \
       src/config/Parser.cpp \
       src/config/SemanticValidator.cpp \
       src/config/ConfigLoader.cpp \
       src/types/ServerConfig.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	@$(CC) $(CFLAGS) -o $(NAME) $(OBJS)
	@echo "$(NAME)"

%.o: %.cpp
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiling $<"

clean:
	@rm -f $(OBJS)
	@echo "Objects removed."

fclean: clean
	@rm -f $(NAME)
	@echo "Executable removed."

re: fclean all

.PHONY: all clean fclean re
