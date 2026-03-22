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
UTILS_SRCS = srcs/utils/ConfigUtils.cpp srcs/utils/ParsingUtils.cpp \
	srcs/utils/Logger.cpp srcs/utils/CgiUtils.cpp srcs/utils/HttpUtils.cpp \
	srcs/utils/StringUtils.cpp

PARSER_SRCS = srcs/config/ConfigParser.cpp srcs/config/ParserSyntaxError.cpp \
	srcs/config/ConfigParserServer.cpp srcs/config/ConfigParserLocation.cpp \

HTTP_SRCS = srcs/http/HttpRequest.cpp srcs/http/HttpResponse.cpp

CGI_SRCS = srcs/cgi/CgiExecutor.cpp srcs/cgi/CgiEnvBuilder.cpp srcs/cgi/CgiPipeManager.cpp

EXAMPLES = srcs/examples.cpp # TODO: Remover na entrega final, apenas para testar funcionalidades

SRCS = srcs/main.cpp $(LEXER_SRCS) $(PARSER_SRCS) $(UTILS_SRCS) $(HTTP_SRCS) $(CGI_SRCS) $(EXAMPLES)

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
