NAME = webserver
all: $(NAME)

CPP := c++
FLAGS := -Wall -Wextra -Werror -std=c++20 -g #remove the -g

SOURCE := src/main.cpp src/server.cpp src/HttpRequest.cpp src/ConfigParse.cpp src/ClientConnection.cpp src/Response.cpp src/ErrorResponseException.cpp
OBJ := $(SOURCE:.cpp=.o)
HEADERS := inc/server.hpp inc/HttpRequest.hpp inc/ConfigParse.hpp inc/ClientConnection.hpp inc/Response.hpp inc/ErrorReponseException.hpp

$(NAME) : $(OBJ)
	$(CPP) $(FLAGS) $(OBJ) -o $(NAME)

src/%.o : src/%.cpp
	$(CPP) $(FLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re