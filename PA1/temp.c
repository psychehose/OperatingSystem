/**********************************************************************
 * Copyright (c) 2021
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>

#include "types.h"
#include "list_head.h"
#include "parser.h"


/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */
 int global_index = 0;
static void dump_history(void);
static char* exec_specifice_history(int target_index);
static int __process_command(char * command);
static int run_command(int nr_tokens, char *tokens[])
{
	if (strcmp(tokens[0], "exit") == 0) return 0;

    else if (strcmp(tokens[0], "cd") == 0) {
        char *home;
        char *path;
        home = getenv("HOME");

        if(strcmp(tokens[1], "~") == 0) {
            int result = chdir(home);

            if(result == -1){
                fprintf(stderr,"cant change\n");
            }
        }
        else if(tokens[1] != NULL) {
            printf("%s\n", tokens[1]);
            int result = chdir(tokens[1]);
            if (result == -1) {
                fprintf(stderr, "cant change\n");
            }
        }
    }

    else if (strcmp(tokens[0], "history") == 0) {
        dump_history();
    }
    else if (strcmp(tokens[0], "!") == 0) {
        pid_t pid = fork();
        if(pid == 0) {
            int i = atoi(tokens[1]);
            __process_command(exec_specifice_history(i));
            waitpid(pid,(int *) 0, 0);

        }


    }


    else {
        pid_t pid = fork();
        switch (pid) {
            case -1:
                fprintf(stderr, "Unable to execute %s\n", tokens[0]);
            case 0:
                execvp(tokens[0],tokens);
            default:
                waitpid(pid,(int *) 0, 0);
        }
    }


	return -EINVAL;
}


/***********************************************************************
 * struct list_head history
 *
 * DESCRIPTION
 *   Use this list_head to store unlimited command history.
 */
LIST_HEAD(history);

struct entry {
    struct list_head list;
    char *string;
    int index;
};


/***********************************************************************
 * append_history()
 *
 * DESCRIPTION
 *   Append @command into the history. The appended command can be later
 *   recalled with "!" built-in command
 */
static void append_history(char * const command)
{
    struct entry *temp = (struct entry *)malloc(sizeof(struct entry));
    temp->string = (char *)malloc(sizeof(char)* strlen(command));
    temp->index = global_index;
    global_index ++;
    strncpy(temp->string, command, strlen(command));
    INIT_LIST_HEAD((&temp->list));
    list_add_tail((&temp->list),&history);
}

static void dump_history(void){
    struct list_head * iterate = NULL;
    struct entry * target = NULL;

    list_for_each(iterate, &history) {
        target = list_entry(iterate, struct entry, list);
        fprintf(stderr, "%2d: %s", target->index, target->string);
    }
}
static char* exec_specifice_history(int target_index) {
    struct list_head * iterate = NULL;
    struct entry * target = NULL;

    list_for_each(iterate, &history) {
        target = list_entry(iterate, struct entry, list);

        if(target->index == target_index) {
            return target->string;
        }
    }
    return NULL;
}


/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
static int initialize(int argc, char * const argv[])
{
	return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
static void finalize(int argc, char * const argv[])
{

}


/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******      */
/*          ****** BUT YOU MAY CALL SOME IF YOU WANT TO.. ******      */
static int __process_command(char * command)
{
	char *tokens[MAX_NR_TOKENS] = { NULL };
	int nr_tokens = 0;

	if (parse_command(command, &nr_tokens, tokens) == 0)
		return 1;

	return run_command(nr_tokens, tokens);
}

static bool __verbose = true;
static const char *__color_start = "[0;31;40m";
static const char *__color_end = "[0m";

static void __print_prompt(void)
{
	char *prompt = "$";
	if (!__verbose) return;

	fprintf(stderr, "%s%s%s ", __color_start, prompt, __color_end);
}

/***********************************************************************
 * main() of this program.
 */
int main(int argc, char * const argv[])
{
	char command[MAX_COMMAND_LEN] = { '\0' };
	int ret = 0;
	int opt;

	while ((opt = getopt(argc, argv, "qm")) != -1) {
		switch (opt) {
		case 'q':
			__verbose = false;
			break;
		case 'm':
			__color_start = __color_end = "\0";
			break;
		}
	}

	if ((ret = initialize(argc, argv))) return EXIT_FAILURE;

	/**
	 * Make stdin unbuffered to prevent ghost (buffered) inputs during
	 * abnormal exit after fork()
	 */
	setvbuf(stdin, NULL, _IONBF, 0);

	while (true) {
		__print_prompt();
		if (!fgets(command, sizeof(command), stdin)) break;

		append_history(command);
		ret = __process_command(command);

		if (!ret) break;
	}

	finalize(argc, argv);

	return EXIT_SUCCESS;
}
