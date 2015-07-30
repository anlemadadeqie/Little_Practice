#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

#define normal		0	/*一般的命令*/
#define out_redirect	1	/*输出重定向*/
#define in_redirect	2	/*输入重定向*/

void print_prompt();						/*打印提示符*/
void get_input(char *);						/*得到输入的命令*/
void explain_input(char *, int *, char a[][256]);		/*对输入的命令进行解析*/
void do_cmd(int, char a[][256]);					/*执行命令*/
int find_command(char *);					/*查找命令中的可执行程序*/

int main(int argc, char **argv)
{
	int	i;
	int	argcount = 0;
	char	arglist[100][256];
	char	**arg = NULL;
	char	*buf =NULL;

	buf = (char *)malloc(256);
	if (buf == NULL)
	{
		perror("malloc failed!");
		exit(-1);
	}
	
	while(1)
	{
	/*将buf所指向的空间清零*/
		memset(buf, 0 , 256);
		print_prompt();
		get_input(buf);
		/*若得到的命令是exit或logout则退出程序*/
		if (strcmp(buf, "exit\n") == 0 || strcmp(buf, "logout\n") == 0)
		break;
		for (i=0; i<100; i++)
		{
			arglist[i][0] = '\0';
		}
		argcount = 0;
		explain_input (buf, &argcount, arglist);
		do_cmd (argcount, arglist);
	}
	
	if (buf!= NULL)
	{
		free(buf);
		buf = NULL;
	}
	
	exit(0);
}

void print_prompt()
{
	printf("myshell$$ ");
}

void get_input (char *buf)
{
	int len = 0;
	int ch;
	ch = getchar();
	while (len < 256 && ch!= '\n')
	{
		buf[len++] = ch;
		ch = getchar();
	}

	if (len == 256)
	{
		printf("command is too long\n");
		exit(-1);
	}

	buf[len] = '\n';
	len++;
	buf[len] = '\0';
}

/*解析buf中的命令,将结果存入arglist中,命令以回车符号\n结束*/
/*如输入命令为"ls -l /tmp",则arglist[0]存的ls,arglist[1]存有-l, arglist[2]存有/tmp*/
void explain_input(char *buf, int *argcount, char arglist[100] [256])
{
	char *p = buf;
	char *q = buf;
	int number = 0;
	
	while (1)
	{
		if (p[0] == '\n')
		break;
		
		if (p[0] == ' ')
		 	p++;
		else 
		{
			q = p;
			number = 0;
			while( (q[0] != ' ') && (q[0] != '\n'))
			{
				number++;
				q++;
			}
			strncpy(arglist[*argcount], p , number +1);
			arglist[*argcount][number] = '\0';
			*argcount = *argcount +1;
			p=q;
		}
	}
}

void do_cmd(int argcount, char arglist[100][256])
{
	int	flag = 0;
	int	how = 0;
	int	background = 0;
	int	i;
	char*	file;
	int	status;
	int	fd;
	char*	arg[argcount+1];
	pid_t	pid;

	/*将命令取出来*/
	for (i=0; i<argcount; i++)
	{
		arg[i] = (char *)arglist[i];
	}
	arg[argcount] = NULL;

	/*查看命令行是否有后台运行符*/
	for (i=0; i<=argcount-2; i++)
	{
		if (strcmp(arg[i], "&") == 0)
		{
			printf("wrong  command\n");
			return;
		}
	}
	for (i=0; i<=argcount-1; i++)
	{
		if (strcmp(arg[i], "&") == 0)
		{
			if ( i == argcount-1)
			{
				background = 1;
				arg[argcount-1] == NULL;
				break;
			}
		}
	}
	
	for (i=0; arg[i] != NULL; i++)
	{
		if (strcmp(arg[i], ">") == 0)
		{
			flag++;
			how = out_redirect;
			if ((arg[i+1] == NULL) || (i == 0))
				flag++;
		}

		if (strcmp (arg[i], "<") == 0)
		{
			flag++;
			how = in_redirect;
			if ((arg[i+1] == NULL) || (i == 0))
				flag++;
		}
	}

/*如果flag大于1,说明命令中含有多个>,< 符号,本程序不支持这样的命令,或者命令的格式不正确*/
	if (flag > 1)
	{
		printf("wrong command\n");
		return;
	}
	if (how == out_redirect)
	{
		for (i=0; arg[i]!=NULL; i++)
		{
			if (strcmp(arg[i],">") == 0)
			{
				file = arg[i+1];
				arg[i] = NULL;
			}
		}
	}
	if (how == in_redirect)
	{
		for (i=0; arg[i]!=NULL; i++)
		{
			if (strcmp(arg[i],"<") == 0)
			{
				file = arg[i+1];
				arg[i] = NULL;
			}
		}
	}
	

	if ((pid = fork()) < 0)
	{
		printf("fork error!\n");
		return;
	}

	switch (how)
	{
		case 0:
			if (pid == 0)
			{
				if (!(find_command (arg[0])))
				{
					printf("%s : command not found\n", arg[0]);
					exit(0);
				}
				execvp(arg[0], arg);
				exit(0);
			}
			break;
		case 1:
			if (pid == 0)
			{
				if (!(find_command (arg[0])))
				{
					printf("%s : command not found\n", arg[0]);
					exit(0);
				}
				fd = open(file, O_RDWR|O_CREAT|O_TRUNC, 0644);
				dup2(fd, 1);
				execvp(arg[0], arg);
				exit(0);
			}
			break;
		case 2:
			if (pid == 0)
			{
				if (!(find_command (arg[0])))
				{
					printf("%s : command not found\n", arg[0]);
					exit(0);
				}
				fd = open(file,O_RDONLY);
				dup2(fd, 1);
				execvp(arg[0], arg);
				exit(0);
			}
			break;
		default:
			break;
	}

	/*若命令中有&,表示后台执行,父进程直接返回,不等待子进程结束*/
	if (background == 1)
	{
		printf("[process id %d]\n", pid);
		return;
	}

	/* 父进程等待子进程结束*/
	if (waitpid (pid, &status, 0) == -1)
		printf("wait for child process error\n");
}

/*查找命令中的可执行程序*/
int find_command (char *command)
{
	DIR *dp;
	struct dirent* dirp;
	char* path[] = { "./", "/bin", "/usr/bin", NULL};
	
	/*使当前目录下的程序可以运行,如命令"./fork"可以被正确解释和执行*/
	if (strncmp(command, "./", 2) == 0)
		command =command+2;
		
	/*分别在当前目录,/bin和/usr/bin目录查找要执行的程序*/
	int i = 0;
	while (path[i] != NULL)
	{
		if ((dp = opendir(path[i])) == NULL)
			printf("can not open /bin\n");
		while ((dirp = readdir(dp)) != NULL)
		{
			if(strcmp(dirp->d_name,command) == 0)
			{
				closedir(dp);
				return 1;
			}
		}
		closedir (dp);
		i++;
	}
	return 0;
}