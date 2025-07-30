NAME = webserver

all: $(NAME)

debug: FLAGS += -fsanitize=address,undefined -g
debug: $(NAME)

CPP := c++
FLAGS := -Wall -Wextra -Werror -std=c++20

DEPFLAGS = -MMD -MP

OBJDIR = obj
DEPDIR = dep

SRC_DIR := src
SRCS_NO_DIR:= main.cpp \
	server.cpp \
	request/HttpRequest.cpp \
	request/cgi.cpp \
	request/doRequest.cpp \
	request/headers.cpp \
	request/paths.cpp \
	request/urlCoding.cpp \
	ConfigParse.cpp \
	ClientConnection.cpp \
	Response.cpp \
	ErrorResponseException.cpp

SRCS := $(addprefix $(SRC_DIR)/, $(SRCS_NO_DIR))

OBJS = $(SRCS_NO_DIR:%.cpp=$(OBJDIR)/%.o)
DEPS = $(SRCS_NO_DIR:%.cpp=$(DEPDIR)/%.d)

$(NAME) : $(OBJS)
	$(CPP) $(FLAGS) -o $(NAME) $(OBJS)

$(OBJDIR)/%.o : $(SRC_DIR)/%.cpp
	@mkdir -p $(@D) $(dir $(DEPDIR)/$*.d)
	$(CPP) $(FLAGS) -c $< -o $@ $(DEPFLAGS) -MF $(DEPDIR)/$*.d

-include $(DEPS)

clean:
	rm -rf $(OBJDIR) $(DEPDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all debug clean fclean re