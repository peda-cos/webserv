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
	srcs/utils/StringUtils.cpp srcs/utils/ParserDirectiveUtils.cpp

PARSER_SRCS = srcs/config/ConfigParser.cpp srcs/config/ParserSyntaxError.cpp \
	srcs/config/ConfigParserServer.cpp srcs/config/ConfigParserLocation.cpp \

SERVER_SRCS = srcs/server/Server.cpp	

HTTP_SRCS = srcs/http/HttpRequest.cpp srcs/http/HttpResponse.cpp srcs/http/HttpResponseBuilder.cpp srcs/http/HttpRequestParser.cpp srcs/http/ChunkedDecoder.cpp srcs/http/MimeTypes.cpp

CGI_SRCS = srcs/cgi/CgiExecutor.cpp srcs/cgi/CgiEnvBuilder.cpp srcs/cgi/CgiPipeManager.cpp srcs/cgi/CgiHandler.cpp

SRCS = srcs/main.cpp $(LEXER_SRCS) $(PARSER_SRCS) $(UTILS_SRCS) $(HTTP_SRCS) $(CGI_SRCS) $(SERVER_SRCS)

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
