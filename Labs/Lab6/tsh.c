/*
 * tsh - A tiny shell program with job control
 *
 * <Put your name and login ID here>
 * 棱镜子 lastprismer
 */
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* Misc manifest constants */
#define MAXLINE 1024   /* max line size */
#define MAXARGS 128    /* max args on a command line */
#define MAXJOBS 16     /* max jobs at any point in time */
#define MAXJID 1 << 16 /* max job ID */

/* 自行定义的常量宏 */
#define INF 0xffffffff

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Parsing states */
#define ST_NORMAL 0x0  /* next token is an argument */
#define ST_INFILE 0x1  /* next token is the input file */
#define ST_OUTFILE 0x2 /* next token is the output file */

/* Global variables */
extern char **environ;   /* defined in libc */
char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
int verbose = 0;         /* if true, print additional output */
int nextjid = 1;         /* next job ID to allocate */
char sbuf[MAXLINE];      /* for composing sprintf messages */

struct job_t {           /* The job struct */
  pid_t pid;             /* job PID */
  int jid;               /* job ID [1, 2, ...] */
  int state;             /* UNDEF, BG, FG, or ST */
  char cmdline[MAXLINE]; /* command line */
};
struct job_t job_list[MAXJOBS]; /* The job list */

struct cmdline_tokens {
  int argc;            /* Number of arguments */
  char *argv[MAXARGS]; /* The arguments list */
  char *infile;        /* The input file */
  char *outfile;       /* The output file */
  enum builtins_t {    /* Indicates if argv[0] is a builtin command */
                    BUILTIN_NONE,
                    BUILTIN_QUIT,
                    BUILTIN_JOBS,
                    BUILTIN_BG,
                    BUILTIN_FG,
                    BUILTIN_KILL,
                    BUILTIN_NOHUP
  } builtins;
};

/* End global variables */

/* Function prototypes */
void eval(char *cmdline);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, struct cmdline_tokens *tok);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *job_list);
int maxjid(struct job_t *job_list);
int addjob(struct job_t *job_list, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *job_list, pid_t pid);
pid_t fgpid(struct job_t *job_list);
struct job_t *getjobpid(struct job_t *job_list, pid_t pid);
struct job_t *getjobjid(struct job_t *job_list, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *job_list, int output_fd);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
ssize_t sio_puts(char s[]);
ssize_t sio_putl(long v);
ssize_t sio_put(const char *fmt, ...);
void sio_error(char s[]);

typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/* 自定义函数 */

pid_t Fork(); /* 错误处理fork */
int Execve(const char *_path, char *const *_argv,
           char *const *_envp);                   /* 错误处理execve */
int Kill(pid_t pid, int sig);                     /* 错误处理kill */
int Open(char *filename, int flags, mode_t mode); /* 错误处理open */
void wait_foreground(pid_t pid);                  /* 等待前台进程结束 */
void do_bgfg(char *const argv[], int job_state);  /* 集成bg,fg命令处理 */
int ratoi(const char *pstr);                      /* 自己写一个atoi */
void kill_command(char *const argv[]);            /* kill job的处理 */
int parse_parameters(char *const argv[], int *_is_pid); /* 解析命令参数 */

/* 自定义函数结束 */

/*
 * main - The shell's main routine
 */
int main(int argc, char **argv) {
  char c;
  char cmdline[MAXLINE]; /* cmdline for fgets */
  int emit_prompt = 1;   /* emit prompt (default) */

  /* Redirect stderr to stdout (so that driver will get all output
   * on the pipe connected to stdout) */
  dup2(1, 2);

  /* Parse the command line */
  while ((c = getopt(argc, argv, "hvp")) != EOF) {
    switch (c) {
    case 'h': /* print help message */
      usage();
      break;
    case 'v': /* emit additional diagnostic info */
      verbose = 1;
      break;
    case 'p':          /* don't print a prompt */
      emit_prompt = 0; /* handy for automatic testing */
      break;
    default:
      usage();
    }
  }

  /* Install the signal handlers */

  /* These are the ones you will need to implement */
  Signal(SIGINT, sigint_handler);   /* ctrl-c */
  Signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
  Signal(SIGCHLD, sigchld_handler); /* Terminated or stopped child */
  Signal(SIGTTIN, SIG_IGN);
  Signal(SIGTTOU, SIG_IGN);

  /* This one provides a clean way to kill the shell */
  Signal(SIGQUIT, sigquit_handler);

  /* Initialize the job list */
  initjobs(job_list);

  /* Execute the shell's read/eval loop */
  while (1) {

    if (emit_prompt) {
      printf("%s", prompt);
      fflush(stdout);
    }
    if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
      app_error("fgets error");
    if (feof(stdin)) {
      /* End of file (ctrl-d) */
      printf("\n");
      fflush(stdout);
      fflush(stderr);
      exit(0);
    }

    /* Remove the trailing newline */
    cmdline[strlen(cmdline) - 1] = '\0';

    /* Evaluate the command line */
    eval(cmdline);

    fflush(stdout);
    fflush(stdout);
  }

  exit(0); /* control never reaches here */
}

/*
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
 */
void eval(char *cmdline) {
  char *execve_filename; /* 可执行文件的名称 */
  int bg;                /* should the job run in bg or fg? */
  int jobstate;          /* job的状态 */
  int command_state;     /* bg命令还是fg命令 */
  int fd = -1;           /* 文件标识符,通用 */
  struct cmdline_tokens tok;

  pid_t pid;             /* 当前管理的进程的pid */
  sigset_t mask_all;     /* 阻塞所有信号的掩码 */
  sigset_t mask_SIGCHLD; /* 阻塞SIGCHLD的掩码 */
  sigset_t mask_SIGHUP;  /* 阻塞SIGHUP的掩码 */
  sigset_t mask_three;   /* 阻塞SIGCHLD,SIGINT,SIGTSTP的掩码 */
  sigset_t prev_mask;    /* 用于备份的掩码 */

  /* 解析命令行 */
  bg = parseline(cmdline, &tok);
  jobstate = (bg == 0) ? FG : BG;
  command_state = (tok.builtins == BUILTIN_BG) ? BG : FG;
  execve_filename = tok.argv[0];

  sigfillset(&mask_all); /* 构建阻塞所有的掩码 */
  sigemptyset(&mask_SIGCHLD);
  sigemptyset(&mask_three);
  sigemptyset(&mask_SIGHUP);
  sigaddset(&mask_SIGCHLD, SIGCHLD); /* 构建阻塞SIGCHLD的掩码 */
  sigaddset(&mask_SIGHUP, SIGHUP);   /* 构建阻塞SIGHUP的掩码 */
  sigaddset(&mask_three, SIGCHLD);
  sigaddset(&mask_three, SIGINT);
  sigaddset(&mask_three, SIGTSTP); /* 构建阻塞SIGCHLD,SIGINT,SIGTSTP的掩码 */

  if (bg == -1)            /* 解析错误 */
    return;                /* 直接返回 */
  if (tok.argv[0] == NULL) /* 无W视空的输入 */
    return;

  if (tok.builtins == BUILTIN_QUIT)
    exit(0); /* 退出tsh */
  else if (tok.builtins == BUILTIN_JOBS) {
    if (tok.outfile != NULL) {
      fd = Open(tok.outfile, O_RDWR | O_CREAT, 0);
      listjobs(job_list, fd);
      close(fd);
    } else {
      listjobs(job_list, STDOUT_FILENO);
    }
    return;
  } else if (tok.builtins == BUILTIN_KILL) {
    kill_command(tok.argv);
    return;
  } else if (tok.builtins == BUILTIN_BG || tok.builtins == BUILTIN_FG) {
    do_bgfg(tok.argv, command_state);
    return;
  }

  sigprocmask(SIG_BLOCK, &mask_three, &prev_mask);

  if ((pid = Fork()) == 0) {
    sigprocmask(SIG_SETMASK, &prev_mask, NULL); /* 不阻塞 */
    setpgid(0, 0);
    if (tok.infile != NULL) {
      fd = Open(tok.infile, O_RDONLY, 0);
      dup2(fd, STDIN_FILENO);
    }
    if (tok.outfile != NULL) {
      fd = Open(tok.outfile, O_RDWR | O_CREAT, 0);
      dup2(fd, STDOUT_FILENO);
    }

    /*DEBUG*/ // sio_puts("SON HERE\n");
    if (tok.builtins == BUILTIN_NOHUP) {
      sigprocmask(SIG_BLOCK, &mask_SIGHUP, &prev_mask); /* 无视SIGHUP */
      execve_filename = tok.argv[1];
    }
    Execve(execve_filename, tok.argv, environ); /* 运行程序,不返回 */
    close(fd);
    exit(0);
  } else { /* 父进程 */
    sigprocmask(SIG_BLOCK, &mask_all,
                NULL); /* 阻塞所有信号,不进入信号处理函数 */

    /*DEBUG*/ // sio_puts("FATHER HERE\n");

    addjob(job_list, pid, jobstate, cmdline);

    /*DEBUG*/ // sio_puts("ADD JOB\n");

    if (jobstate == FG) {
      wait_foreground(pid);
    } else {
      fprintf(stdout, "[%d] (%d) %s\n", pid2jid(pid), pid,
              cmdline); /* 阻塞状态下,不怕被打断 */
    }
    sigprocmask(SIG_SETMASK, &prev_mask, NULL); /* 恢复 */
    return;
  }
}

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Parameters:
 *   cmdline:  The command line, in the form:
 *
 *                command [arguments...] [< infile] [> oufile] [&]
 *
 *   tok:      Pointer to a cmdline_tokens structure. The elements of this
 *             structure will be populated with the parsed tokens. Characters
 *             enclosed in single or double quotes are treated as a single
 *             argument.
 * Returns:
 *   1:        if the user has requested a BG job
 *   0:        if the user has requested a FG job
 *  -1:        if cmdline is incorrectly formatted
 *
 * Note:       The string elements of tok (e.g., argv[], infile, outfile)
 *             are statically allocated inside parseline() and will be
 *             overwritten the next time this function is invoked.
 */
int parseline(const char *cmdline, struct cmdline_tokens *tok) {

  static char array[MAXLINE];        /* holds local copy of command line */
  const char delims[10] = " \t\r\n"; /* argument delimiters (white-space) */
  char *buf = array;                 /* ptr that traverses command line */
  char *next;                        /* ptr to the end of the current arg */
  char *endbuf;                      /* ptr to end of cmdline string */
  int is_bg;                         /* background job? */

  int parsing_state; /* indicates if the next token is the
                        input or output file */

  if (cmdline == NULL) {
    (void)fprintf(stderr, "Error: command line is NULL\n");
    return -1;
  }

  (void)strncpy(buf, cmdline, MAXLINE);
  endbuf = buf + strlen(buf);

  tok->infile = NULL;
  tok->outfile = NULL;

  /* Build the argv list */
  parsing_state = ST_NORMAL;
  tok->argc = 0;

  while (buf < endbuf) {
    /* Skip the white-spaces */
    buf += strspn(buf, delims);
    if (buf >= endbuf)
      break;

    /* Check for I/O redirection specifiers */
    if (*buf == '<') {
      if (tok->infile) {
        (void)fprintf(stderr, "Error: Ambiguous I/O redirection\n");
        return -1;
      }
      parsing_state |= ST_INFILE;
      buf++;
      continue;
    }
    if (*buf == '>') {
      if (tok->outfile) {
        (void)fprintf(stderr, "Error: Ambiguous I/O redirection\n");
        return -1;
      }
      parsing_state |= ST_OUTFILE;
      buf++;
      continue;
    }

    if (*buf == '\'' || *buf == '\"') {
      /* Detect quoted tokens */
      buf++;
      next = strchr(buf, *(buf - 1));
    } else {
      /* Find next delimiter */
      next = buf + strcspn(buf, delims);
    }

    if (next == NULL) {
      /* Returned by strchr(); this means that the closing
         quote was not found. */
      (void)fprintf(stderr, "Error: unmatched %c.\n", *(buf - 1));
      return -1;
    }

    /* Terminate the token */
    *next = '\0';

    /* Record the token as either the next argument or the i/o file */
    switch (parsing_state) {
    case ST_NORMAL:
      tok->argv[tok->argc++] = buf;
      break;
    case ST_INFILE:
      tok->infile = buf;
      break;
    case ST_OUTFILE:
      tok->outfile = buf;
      break;
    default:
      (void)fprintf(stderr, "Error: Ambiguous I/O redirection\n");
      return -1;
    }
    parsing_state = ST_NORMAL;

    /* Check if argv is full */
    if (tok->argc >= MAXARGS - 1)
      break;

    buf = next + 1;
  }

  if (parsing_state != ST_NORMAL) {
    (void)fprintf(stderr, "Error: must provide file name for redirection\n");
    return -1;
  }

  /* The argument list must end with a NULL pointer */
  tok->argv[tok->argc] = NULL;

  if (tok->argc == 0) /* ignore blank line */
    return 1;

  if (!strcmp(tok->argv[0], "quit")) { /* quit command */
    tok->builtins = BUILTIN_QUIT;
  } else if (!strcmp(tok->argv[0], "jobs")) { /* jobs command */
    tok->builtins = BUILTIN_JOBS;
  } else if (!strcmp(tok->argv[0], "bg")) { /* bg command */
    tok->builtins = BUILTIN_BG;
  } else if (!strcmp(tok->argv[0], "fg")) { /* fg command */
    tok->builtins = BUILTIN_FG;
  } else if (!strcmp(tok->argv[0], "kill")) { /* kill command */
    tok->builtins = BUILTIN_KILL;
  } else if (!strcmp(tok->argv[0], "nohup")) { /* kill command */
    tok->builtins = BUILTIN_NOHUP;
  } else {
    tok->builtins = BUILTIN_NONE;
  }

  /* Should the job run in the background? */
  if ((is_bg = (*tok->argv[tok->argc - 1] == '&')) != 0)
    tok->argv[--tok->argc] = NULL;

  return is_bg;
}

/*****************
 * Signal handlers
 *****************/

/*
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP, SIGTSTP, SIGTTIN or SIGTTOU signal. The
 *     handler reaps all available zombie children, but doesn't wait
 *     for any other currently running children to terminate.
 */
void sigchld_handler(int sig) {
  int olderrno = errno; /* 保存errno */
  pid_t pid;            /* 后面用来放子进程的pid */
  int status;           /* wait的状态记录 */
  sigset_t mask_all;    /* 阻塞所有信号的掩码 */
  sigset_t prev_mask;   /* 用于备份的掩码 */
  struct job_t *job_ptr;
  sigfillset(&mask_all);

  /*DEBUG*/ // sio_puts("SIGCHLD\n");

  while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG | WCONTINUED)) >
         0) { /* 循环杀子进程 */
    sigprocmask(SIG_BLOCK, &mask_all, &prev_mask);
    /* DEBUG */              // printf("激活,pid:%d\n", pid);
    if (WIFEXITED(status)) { /* 正常终止 */
      deletejob(job_list, pid);
    } else if (WIFSIGNALED(status)) { /* 因为SIGINT终止 */
      sio_put("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid,
              WTERMSIG(status));
      deletejob(job_list, pid);
    } else if (WIFSTOPPED(status)) { /* 因为SIGTSTP停止 */
      sio_put("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid,
              WSTOPSIG(status));
      job_ptr = getjobpid(job_list, pid);
      job_ptr->state = ST;
    } else {
      job_ptr = getjobpid(job_list, pid);
      job_ptr->state = BG;
    }
    sigprocmask(SIG_SETMASK, &prev_mask, NULL);
  }
  errno = olderrno;
  return;
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.
 */
void sigint_handler(int sig) {
  int olderrno = errno; /* 保存errno */
  pid_t foreground_pid; /* 前台子进程的pid */
  sigset_t mask_all;    /* 阻塞所有信号的掩码 */
  sigset_t prev_mask;   /* 用于备份的掩码 */

  sigfillset(&mask_all);
  sigprocmask(SIG_BLOCK, &mask_all, &prev_mask); /* 阻塞所有信号 */
  foreground_pid = fgpid(job_list);
  if (foreground_pid != 0) {
    Kill(-foreground_pid, SIGINT);
  } else {
    sio_puts("fgpid error\n");
    exit(0);
  }
  sigprocmask(SIG_SETMASK, &prev_mask, NULL); /* 解除信号阻塞 */
  errno = olderrno;
  return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.
 */
void sigtstp_handler(int sig) {
  int olderrno = errno; /* 保存errno */
  pid_t foreground_pid; /* 前台子进程的pid */
  sigset_t mask_all;    /* 阻塞所有信号的掩码 */
  sigset_t prev_mask;   /* 用于备份的掩码 */

  sigfillset(&mask_all);
  sigprocmask(SIG_BLOCK, &mask_all, &prev_mask); /* 阻塞所有信号 */
  foreground_pid = fgpid(job_list);

  if (foreground_pid != 0) {
    Kill(-foreground_pid, SIGTSTP);
  } else {
    sio_puts("fgpid error\n");
    exit(0);
  }
  sigprocmask(SIG_SETMASK, &prev_mask, NULL); /* 解除信号阻塞 */
  errno = olderrno;
  return;
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) {
  sio_error("Terminating after receipt of SIGQUIT signal\n");
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
  job->pid = 0;
  job->jid = 0;
  job->state = UNDEF;
  job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *job_list) {
  int i;

  for (i = 0; i < MAXJOBS; i++)
    clearjob(&job_list[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *job_list) {
  int i, max = 0;

  for (i = 0; i < MAXJOBS; i++)
    if (job_list[i].jid > max)
      max = job_list[i].jid;
  return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *job_list, pid_t pid, int state, char *cmdline) {
  int i;

  if (pid < 1)
    return 0;

  for (i = 0; i < MAXJOBS; i++) {
    if (job_list[i].pid == 0) {
      job_list[i].pid = pid;
      job_list[i].state = state;
      job_list[i].jid = nextjid++;
      if (nextjid > MAXJOBS)
        nextjid = 1;
      strcpy(job_list[i].cmdline, cmdline);
      if (verbose) {
        printf("Added job [%d] %d %s\n", job_list[i].jid, job_list[i].pid,
               job_list[i].cmdline);
      }
      return 1;
    }
  }
  printf("Tried to create too many jobs\n");
  return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *job_list, pid_t pid) {
  int i;

  if (pid < 1)
    return 0;

  for (i = 0; i < MAXJOBS; i++) {
    if (job_list[i].pid == pid) {
      clearjob(&job_list[i]);
      nextjid = maxjid(job_list) + 1;
      return 1;
    }
  }
  return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *job_list) {
  int i;

  for (i = 0; i < MAXJOBS; i++)
    if (job_list[i].state == FG)
      return job_list[i].pid;
  return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *job_list, pid_t pid) {
  int i;

  if (pid < 1)
    return NULL;
  for (i = 0; i < MAXJOBS; i++)
    if (job_list[i].pid == pid)
      return &job_list[i];
  return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *job_list, int jid) {
  int i;

  if (jid < 1)
    return NULL;
  for (i = 0; i < MAXJOBS; i++)
    if (job_list[i].jid == jid)
      return &job_list[i];
  return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) {
  int i;

  if (pid < 1)
    return 0;
  for (i = 0; i < MAXJOBS; i++)
    if (job_list[i].pid == pid) {
      return job_list[i].jid;
    }

  /* DEBUG */ sio_puts("NOT FOUND PID IN JOG LIST\n");

  return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *job_list, int output_fd) {
  int i;
  char buf[MAXLINE << 2];

  for (i = 0; i < MAXJOBS; i++) {
    memset(buf, '\0', MAXLINE);
    if (job_list[i].pid != 0) {
      sprintf(buf, "[%d] (%d) ", job_list[i].jid, job_list[i].pid);
      if (write(output_fd, buf, strlen(buf)) < 0) {
        fprintf(stderr, "Error writing to output file\n");
        exit(1);
      }
      memset(buf, '\0', MAXLINE);
      switch (job_list[i].state) {
      case BG:
        sprintf(buf, "Running    ");
        break;
      case FG:
        sprintf(buf, "Foreground ");
        break;
      case ST:
        sprintf(buf, "Stopped    ");
        break;
      default:
        sprintf(buf, "listjobs: Internal error: job[%d].state=%d ", i,
                job_list[i].state);
      }
      if (write(output_fd, buf, strlen(buf)) < 0) {
        fprintf(stderr, "Error writing to output file\n");
        exit(1);
      }
      memset(buf, '\0', MAXLINE);
      sprintf(buf, "%s\n", job_list[i].cmdline);
      if (write(output_fd, buf, strlen(buf)) < 0) {
        fprintf(stderr, "Error writing to output file\n");
        exit(1);
      }
    }
  }
}
/******************************
 * end job list helper routines
 ******************************/

/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) {
  printf("Usage: shell [-hvp]\n");
  printf("   -h   print this message\n");
  printf("   -v   print additional diagnostic information\n");
  printf("   -p   do not emit a command prompt\n");
  exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg) {
  fprintf(stdout, "%s: %s\n", msg, strerror(errno));
  exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg) {
  fprintf(stdout, "%s\n", msg);
  exit(1);
}

/* Private sio_functions */
/* sio_reverse - Reverse a string (from K&R) */
static void sio_reverse(char s[]) {
  int c, i, j;

  for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
    c = s[i];
    s[i] = s[j];
    s[j] = c;
  }
}

/* sio_ltoa - Convert long to base b string (from K&R) */
static void sio_ltoa(long v, char s[], int b) {
  int c, i = 0;

  do {
    s[i++] = ((c = (v % b)) < 10) ? c + '0' : c - 10 + 'a';
  } while ((v /= b) > 0);
  s[i] = '\0';
  sio_reverse(s);
}

/* sio_strlen - Return length of string (from K&R) */
static size_t sio_strlen(const char s[]) {
  int i = 0;

  while (s[i] != '\0')
    ++i;
  return i;
}

/* sio_copy - Copy len chars from fmt to s (by Ding Rui) */
void sio_copy(char *s, const char *fmt, size_t len) {
  if (!len)
    return;

  for (size_t i = 0; i < len; i++)
    s[i] = fmt[i];
}

/* Public Sio functions */
ssize_t sio_puts(char s[]) /* Put string */
{
  return write(STDOUT_FILENO, s, sio_strlen(s));
}

ssize_t sio_putl(long v) /* Put long */
{
  char s[128];

  sio_ltoa(v, s, 10); /* Based on K&R itoa() */
  return sio_puts(s);
}

ssize_t sio_put(const char *fmt, ...) // Put to the console. only understands %d
{
  va_list ap;
  char str[MAXLINE]; // formatted string
  char arg[128];
  const char *mess = "sio_put: Line too long!\n";
  int i = 0, j = 0;
  int sp = 0;
  int v;

  if (fmt == 0)
    return -1;

  va_start(ap, fmt);
  while (fmt[j]) {
    if (fmt[j] != '%') {
      j++;
      continue;
    }

    sio_copy(str + sp, fmt + i, j - i);
    sp += j - i;

    switch (fmt[j + 1]) {
    case 0:
      va_end(ap);
      if (sp >= MAXLINE) {
        write(STDOUT_FILENO, mess, sio_strlen(mess));
        return -1;
      }

      str[sp] = 0;
      return write(STDOUT_FILENO, str, sp);

    case 'd':
      v = va_arg(ap, int);
      sio_ltoa(v, arg, 10);
      sio_copy(str + sp, arg, sio_strlen(arg));
      sp += sio_strlen(arg);
      i = j + 2;
      j = i;
      break;

    case '%':
      sio_copy(str + sp, "%", 1);
      sp += 1;
      i = j + 2;
      j = i;
      break;

    default:
      sio_copy(str + sp, fmt + j, 2);
      sp += 2;
      i = j + 2;
      j = i;
      break;
    }
  } // end while

  sio_copy(str + sp, fmt + i, j - i);
  sp += j - i;

  va_end(ap);
  if (sp >= MAXLINE) {
    write(STDOUT_FILENO, mess, sio_strlen(mess));
    return -1;
  }

  str[sp] = 0;
  return write(STDOUT_FILENO, str, sp);
}

void sio_error(char s[]) /* Put error message and exit */
{
  sio_puts(s);
  _exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) {
  struct sigaction action, old_action;

  action.sa_handler = handler;
  sigemptyset(&action.sa_mask); /* block sigs of type being handled */
  action.sa_flags = SA_RESTART; /* restart syscalls if possible */

  if (sigaction(signum, &action, &old_action) < 0)
    unix_error("Signal error");
  return (old_action.sa_handler);
}

/* 自定义函数区 */

/*
 * Fork - 封装fork()与异常处理
 */

pid_t Fork() {
  pid_t pid = fork();
  if (pid < 0)
    unix_error("Fork error");
  return pid;
}

/*
 * Execve - 封装execve()与异常处理
 */
int Execve(const char *_path, char *const *_argv, char *const *_envp) {
  int result = execve(_path, _argv, _envp);
  if (result < 0) {
    sio_put(_argv[0]);
    sio_puts(": Command not found\n");
    _exit(1);
  }
  return result;
}

/*
 * Open - 封装open()与异常处理
 */
int Open(char *filename, int flags, mode_t mode) {
  int result = open(filename, flags, mode);
  if (result < 0) {
    sio_puts("Error: ");
    sio_puts(filename);
    sio_puts(" No such file or directory\n");
  }
  return result;
}

/*
 * Kill - 封装kill()与异常处理
 */
int Kill(pid_t pid, int sig) {
  int olderrno = errno;
  int result = kill(pid, sig);

  if (result < 0) {
    switch (errno) {
    case EINVAL:
      sio_putl(sig);
      sio_puts(": 指定的信号码无效\n");
      _exit(1);
    case ESRCH:
      sio_putl(pid);
      sio_puts(": 指定的进程或进程组不存在\n");
      _exit(1);
    case EPERM:
      sio_puts("权限不足,无法将信号传递给指定进程\n");
      _exit(1);
    default:
      sio_puts("美味……\n");
      _exit(1);
      break;
    }
  }
  errno = olderrno;
  return result;
}

/*
 * wait_foreground - 等待tsh的前台进程终止
 *
 * 参数:
 *   pid:  要等待的前台子进程
 */
void wait_foreground(pid_t pid) {
  sigset_t empty_mask;
  sigemptyset(&empty_mask); /* pause时不能阻塞信号 */
  while (fgpid(job_list)) {
    sigsuspend(&empty_mask);
  }
}

/*
 * do_bgfg - 处理bg和fg命令
 *
 * 参数:
 *   argv:      解析后得到的命令行参数
 *   job_state: bg或者fg
 */
void do_bgfg(char *const argv[], int job_state) {
  int olderrno = errno;
  int is_pid;
  int process_id;
  struct job_t *job_ptr = NULL;

  process_id = parse_parameters(argv, &is_pid);
  if (is_pid)
    job_ptr = getjobpid(job_list, process_id);
  else
    job_ptr = getjobjid(job_list, process_id);

  if (!job_ptr) {
    if (is_pid) {
      sio_put("(%d): No such process\n", ratoi(argv[1]));
    } else {
      sio_put(argv[1]);
      sio_puts(": No such job\n");
    }
    errno = olderrno;
    return;
  }

  job_ptr->state = job_state;   // 重新设置状态
  kill(-job_ptr->pid, SIGCONT); // 重启进程

  /* DEBUG */ // sio_put("change pid %d to bg, %d\n", job_ptr->pid, job_state);

  if (job_state == BG) {
    sio_put("[%d] (%d) ", job_ptr->jid, job_ptr->pid);
    sio_puts(job_ptr->cmdline);
    sio_puts("\n");
  }
  if (job_state == FG) {
    wait_foreground(job_ptr->pid);
    errno = olderrno;
  }
  errno = olderrno;
  return;
}

/*
 * ratoi - 可重入的简化版atoi
 *
 * 参数:
 *   psyt:  要转换的字符串
 */
int ratoi(const char *pstr) {
  int ret = 0;
  int sign = 1;
  /* 不用检测指针,前面检测过了 */
  if (*pstr == '-') {
    sign = -1;
    pstr++;
  }
  while (*pstr >= '0' && *pstr <= '9') {
    ret = ret * 10 + *pstr - '0';
    pstr++;
  }
  ret = sign * ret;
  return ret;
}

/*
 * kill_command - kill,命令
 *
 * 参数:
 *   argv:   命令行参数
 */
void kill_command(char *const argv[]) {
  int olderrno = errno;
  int is_pid;
  int process_id;
  int jid_sign = 1; /* jid的符号 */
  struct job_t *job_ptr = NULL;

  process_id = parse_parameters(argv, &is_pid);
  if (is_pid)
    job_ptr = getjobpid(job_list, process_id);
  else {
    if (process_id < 0)
      jid_sign = -1;
    job_ptr = getjobjid(job_list, process_id * jid_sign);
  }

  if (!job_ptr) {
    if (is_pid) {
      sio_put("(%d): No such process\n", ratoi(argv[1]));
    } else {
      sio_put(argv[1]);
      sio_puts(": No such job\n");
    }
    errno = olderrno;
    return;
  }
  if (is_pid)
    kill(process_id, SIGTERM);
  else {
    kill(-job_ptr->pid * jid_sign, SIGTERM);
  }
  errno = olderrno;
  return;
}

/*
 * parse_parameters - 解析kill, job命令的参数
 *
 * 参数:
 *   argv:   命令行参数
 *   _is_pid: 如果不为空，存储这个参数是否是pid
 *
 * 返回值:
 *   0xffffffff: 解析错误
 *   其他:       正常值
 */
int parse_parameters(char *const argv[], int *_is_pid) {
  int is_pid;
  int is_jid;
  int result;

  if (!argv[1]) {
    sio_put(argv[0]);
    sio_put("ommand requires PID or %%jobid argument\n");
    return INF;
  }

  is_pid = (argv[1] && argv[1][0] >= '0' && argv[1][0] <= '9');
  is_jid = (argv[1] && argv[1][0] == '%');

  if (!is_pid && !is_jid) {
    sio_put(argv[0]);
    sio_put("argument must be a PID or %%jobid\n");
    return INF;
  }

  if (is_pid)
    result = ratoi(argv[1]);
  else
    result = ratoi(argv[1] + 1);

  if (_is_pid)
    *_is_pid = is_pid;
  return result;
}