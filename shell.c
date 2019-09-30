#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <stdarg.h>
#include <syscall.h>
#include <sys/stat.h>//for mkdir

#define BUFF 1024
#define DELIMS " \n\t\r\a"


/*
char *get_input()
{
	char *input = NULL;
	size_t buff = 0;
	getline(&input, &buff, stdin);//getline() will allocate the buffer
	return input;
}*/

char *builtins[] = {
	"cd", 
	"help", 
	"exit",
	"dbg",
	"pwd",
	"mkdir",
	"rename"
	};

int shell_cd(char **args);
int shell_exit(char **args);
int shell_help(char **args);
int shell_dbg(char **args);
int shell_pwd(char **args);
int shell_mkdir(char **args);
int shell_rename(char **args);

int (*buildin_func[])(char **) = {
	&shell_cd,
	&shell_help,
	&shell_exit,
	&shell_dbg,
	&shell_pwd,
	&shell_mkdir,
	&shell_rename
};

int shell_number_bultins()
{
	return sizeof(builtins) / sizeof(char*);
}

/*The implementation of the bultin functions*/

/*********dbg********/
void printmsg(const char* format, ...)
{
    va_list ap;
    fprintf(stdout, "[%d] ", getpid());
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
}

void run_debugee(char *program)
{
	printf("target started, will run %s\n", program);
	
	if(program==NULL)
	{
		return;
	}
	//Allow tracing of the process
	if(ptrace(PTRACE_TRACEME, 0, 0, 0)<0)
	{
		perror("ptrace");
		return;
	}
	
	if( execl(program, program, NULL) == -1)
	{
		perror("execl");
		return;
	}
	
	//execl(program, program, NULL);
}

void run_debugger(pid_t child_pid)
{
	int status;
	unsigned int instruction_counter = 0;

	printf("debugger started\n");
	
	//wait for the child to stop at its first instruction
	wait(&status);
	
	do
	{
		instruction_counter++;
		struct user_regs_struct regs;
		ptrace(PTRACE_GETREGS, child_pid, 0, &regs);
		unsigned instr = ptrace(PTRACE_PEEKTEXT, child_pid, regs.rip, 0);
		
		printmsg("instruction_counter = %u.  RIP = 0x%08x.  instr = 0x%08x\n",
                    instruction_counter, regs.rip, instr);
		
		if(WIFSTOPPED(status))
		{
			fprintf(stdout, "Child %ld has received the signal  %d\n", child_pid, WTERMSIG(status));
		}
		
		if(WIFSTOPPED(status))
		{
			fprintf(stdout, "Child has stopped due to signal %ld\n", WSTOPSIG(status));
		}
		
		if(ptrace(PTRACE_SINGLESTEP, child_pid, 0, 0)<0)
		{
			perror("ptrace");
		}
		
		wait(&status);
	}while(!WIFEXITED(status));
	
	fprintf(stdout, "Child executed %d instructions\n", instruction_counter);
	
}

int shell_dbg(char **argv)
{
	pid_t child_pid;
	
	child_pid = fork();
	
	if(argv[1]==NULL)
	{
		perror("Argument please\n");
	}
	
	if(child_pid==0)//child process
		run_debugee(argv[1]);
	else if(child_pid>0)//parent process
		run_debugger(child_pid);
	else
	{
		fprintf(stderr,"Could not create new process");
	}
	
	return 1;
	
}


/**********other functions********/

int shell_rename(char **args)
{
	if(args[1]!=NULL)
	{
		if(args[2]==NULL)
		{
			fprintf(stderr, "rename expects two arguments\n");
		}
		
		else
		{
			if(rename(args[1], args[2])!=0)
			{
				perror("Fail to rename the file");
			}
		}
	}
	
	else
	{
		fprintf(stderr, "Rename expects two arguments, none given\n");
	}
	
	return 1;
}

int shell_mkdir(char **args)
{
	if(args[1]==NULL)
	{
		fprintf(stderr, "Shell expects argument to mkdir\n");
	}
	
	if(mkdir(args[1], S_IRUSR | S_IWUSR | S_IXUSR)!=0)//a new directory that can be R, W, Ex.
	{
		perror("Failed to create new directory");
	}
	
	return 1;
}

int shell_pwd(char **args)
{
	if(args[1]!=NULL)
	{
		fprintf(stderr, "pwd expects no argument\n");
	}
	else
	{
		char dir[1024];
		if(getcwd(dir, sizeof(dir))!=NULL)
			fprintf(stdout, "The current working directory is %s\n", dir);
		else
			perror("error-->getcwd()");
	}
	return 1;
}

int shell_cd(char **args)
{
	if(args[1]==NULL)
	{
		fprintf(stderr, "shell expected argument to \"cd\"\n");
	}
	else
	{
		if(chdir(args[1])!=0)
		{
			perror("shell->cd->chdir");
		}
	}
	
	return 1;
}

int shell_help(char **args)
{
	int i, nr_buildins;
	printf("This is my shell, the list of builtin-commands is:\n");
	nr_buildins = shell_number_bultins();
	for(i=0;i<nr_buildins;i++)
	{
		printf("%s\n", builtins[i]);
	}
	return 1;
}

int shell_exit(char **args)
{
	return 0;
}

/****the bare shell mechanism****/

char *get_input()
{
	char c;
	int buffsize = 1024, poz = 0;
	char *input;
	
	input = (char *)malloc(buffsize*sizeof(char));
	
	if(!input)
	{
		perror("Memory allocation error");
		exit(EXIT_FAILURE);
	}
	
	while(1)
	{
		c = getchar();
		if(c == EOF || c == '\n')
		{
			input[poz] = '\0';
			return input;
		}
		
		else
		{
			input[poz] = c;
		}
		poz++;
		
		if(poz>buffsize)
		{
			buffsize += buffsize;
			input = (char *)realloc(input, buffsize);
			if(!input)
			{
				fprintf(stderr, "Memory allocation error");
				exit(EXIT_FAILURE);
			}
		}
	}
}

char **parse(char *input)
{
	char *token;
	char **tokens;//**tokens == a null terminated array of pointer
	int buffsize = BUFF;
	int position = 0;
	
	tokens = (char **)malloc(buffsize*sizeof(char*));
	
	if(!tokens)
	{
		fprintf(stderr, "Allocation error");
		exit(EXIT_FAILURE);
	}
	
	token = strtok(input, DELIMS);
	
	while(token!=NULL)
	{
		tokens[position] = token;
		position++;
		
		if(position > buffsize)
		{
			buffsize = buffsize + BUFF;
			tokens = (char **)realloc(tokens, buffsize);
		}
		
		if(!tokens)
		{
			fprintf(stderr, "Allocation error");
			exit(EXIT_FAILURE);
		}
		
		token = strtok(NULL, DELIMS);
	}
	
	tokens[position] = NULL;
	return tokens;
}

int launch(char **args)
{
	pid_t child_pid;
	int status;
	/* pid -> the PID of the child*/
	
	child_pid = fork();
	if(child_pid<0)
	{
		perror("shell-->error forking");
		exit(EXIT_FAILURE);
	}
	if(child_pid==0)//child process
	{
			if(execvp(args[0], args)==-1)
			{
				perror("execvp");
				exit(EXIT_FAILURE);
			}
			//execvp(args[0], args);
	}
	else//make the parent wait until the child is over
	{
		do
		{
			waitpid(child_pid, &status, WUNTRACED);
		}while(!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	
	return 1;
}

int execute(char **args)
{
	int i,nr_buildins;
	
	if(args[0]==NULL)
	{
		return 1;
	}
	
	nr_buildins = shell_number_bultins();
	for(i=0;i<nr_buildins;i++)
	{
		if(strcmp(args[0], builtins[i])==0)
			return (*buildin_func[i])(args);
	}
	
	return launch(args);
}

void shell_loop()
{
	int status;
	char *input;
	char **commands;
	do
	{
		printf("$>>>");
		input = get_input();
		commands = parse(input);
		status = execute(commands);
		
		free(input);
		free(commands);
		
	}while(status);
}

int main()
{
	printf("This is where the shell begins\n");
	
	shell_loop();
	
	return EXIT_SUCCESS;
}
