# Name of the library
NAME		=	decoder

# Compiler and flags
CC			=	clang
CFLAGS		=	-O0 -Wall -Wextra -Werror main.c -o decoder -lpthread
RM			=	rm -f


# Rules
all: $(NAME)

# Link object files and libft to create the final executable
$(NAME):
	gcc -O0 -Wall -Wextra -Werror main.c -o decoder -lpthread
	@echo "Compiling $(NAME) project"

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
