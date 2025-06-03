NAME = webserver
all: $(NAME)

CPP := c++
FLAGS := -Wall -Wextra -Werror -std=c++11

SOURCE := src/main.cpp src/server.cpp src/HttpRequest.cpp src/ConfigParse.cpp
OBJ := $(SOURCE:.cpp=.o)
HEADERS := inc/server.hpp inc/HttpRequest.hpp inc/ConfigParse.cpp

$(NAME) : $(OBJ)
	$(CPP) $(FLAGS) $(OBJ) -o $(NAME) 

%.o : %.cpp $(HEADERS)
	$(CPP) $(FLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
