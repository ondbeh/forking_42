# Name of the library
NAME			=	decoder
NAME_DEBUG		=	decoder_debug

# Compiler and flags
CC				=	clang
CFLAGS_DEBUG	=	-O0 -Wall -Wextra -Werror -lpthread
CFLAGS_DEBUG	=	-O0 -Wall -Wextra -Werror -lpthread -g -fsanitize=address
RM				=	rm -f


# Rules
all: $(NAME)

# Link object files and libft to create the final executable
$(NAME):
	$(CC) $(CFLAGS) main.c -o $(NAME)
	@echo "Compiling $(NAME) project"

debug:
	$(CC) $(CFLAGS_DEBUG) main.c -o $(NAME_DEBUG)
	@echo "Compiling $(NAME) project with debug flags"

# Clean object files
clean:
	@rm -rf $(OBJ_DIR)
	@echo "Deleting $(NAME) objects"

# Full clean: also remove the executable and libft objects
fclean: clean
	@$(RM) $(NAME)
	@echo "Deleting $(NAME) executable"

# Rebuild everything
re: fclean all

# PHONY prevents conflicts with files named like the targets
.PHONY: all clean fclean re
