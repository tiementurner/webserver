NAME =	webserv

INC =	WebServer/ServerManager.hpp \
		WebServer/Server.hpp \
		WebServer/ResponseBuilder.hpp \
		WebServer/CgiHandler/CgiHandler.hpp \
		WebServer/HttpParser/RequestParser.hpp \
		WebServer/ConfigParser/ConfigParser.hpp \
		WebServer/ConfigParser/ServerConfig.hpp \
		WebServer/Utilities/Utilities.hpp

OBJ =	Main.o \
		WebServer/ServerManager.o \
		WebServer/Server.o \
		WebServer/ResponseBuilder.o \
		WebServer/CgiHandler/CgiHandler.o \
		WebServer/HttpParser/RequestParser.o \
		WebServer/ConfigParser/ConfigParser.o \
		WebServer/ConfigParser/ServerConfig.o \
		WebServer/Utilities/Utilities.o

CC =	clang++

CCFLAGS = -Wall -Wextra -Werror

all: $(NAME)

$(NAME): $(OBJ)
		$(CC) $(CCFLAGS) $(OBJ) -o $@

%.o: %.cpp $(INC)
		$(CC) -c $(CCFLAGS) -o $@ $<

clean:
		@rm -rf $(OBJ)

fclean: clean
		@rm -rf $(NAME)

re:
		@make fclean
		@make all

.PHONY: all clean fclean re