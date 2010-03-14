/* 
Setuid Helper - ARCH notes The Code Checker process runs as a non-root
user (checker). To run the submission programs as a different user
than 'checker', we use setuid_helper.c to achieve it. It is a setuid root
executable which limits CPU time, memory usage, forks and maximum file size
for its child process. 
*/

#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#define NO_OPTS 7
#define MAX_PATH_SIZE 300

pid_t p;

static void alarm_handler(int signo) 
{
  /* its time to kill the child now! */
  kill(p, SIGKILL);
}

struct sigaction alarm_act;
char *opts[NO_OPTS] = {"debug", "memlimit", "timelimit", "maxfilesize",
                         "infile", "outfile", "errfile"};

int main(int argc, char* argv[]) {
/* Usage: setuid_helper debug=<bool> timelimit=<secs> memlimit=<MB>
  maxfilesize=<MB> infile=<name> outfile=<name> errfile=<name> <submission-exec>
  No error checking done, since this will be run only by the Code checker. */

  int i, debug, memlimit, timelimit, maxfilesize;
  char infile[MAX_PATH_SIZE], outfile[MAX_PATH_SIZE], errfile[MAX_PATH_SIZE];
  for(i=1; i< argc; i++) 
  {
    char *name = strtok(argv[i], "="),
         *value = strtok(NULL, "=");
    if (!strncmp(name, opts[0], strlen(opts[0]))) debug = value[0] - '0';
    else if (!strncmp(name, opts[1], strlen(opts[1]))) memlimit = atoi(value);
    else if (!strncmp(name, opts[2], strlen(opts[2]))) timelimit = atoi(value);
    else if (!strncmp(name, opts[3], strlen(opts[3]))) maxfilesize = atoi(value);
    else if (!strncmp(name, opts[4], strlen(opts[4]))) strcpy(infile, value);
    else if (!strncmp(name, opts[5], strlen(opts[5]))) strcpy(outfile, value);
    else if (!strncmp(name, opts[6], strlen(opts[6]))) strcpy(errfile, value);
  }

  // fork argv[1]
  p = fork();
  if (!p) { 
    freopen(infile, "r", stdin); 
    freopen(outfile, "w", stdout);
    freopen(errfile, "w", stderr);

    //drop priveleges
    setuid(1002);    

    struct rlimit lim ;
    // set limit on number of forks possible.
    lim.rlim_cur = lim.rlim_max = 0; 
    int ret = setrlimit(RLIMIT_NPROC, &lim);

    lim.rlim_cur = lim.rlim_max = memlimit << 20;
    ret = setrlimit(RLIMIT_AS, &lim);

    lim.rlim_cur = timelimit; lim.rlim_max = timelimit + 1;
    ret = setrlimit(RLIMIT_CPU, &lim);

    lim.rlim_cur = lim.rlim_max = maxfilesize << 20;
    ret = setrlimit(RLIMIT_FSIZE, &lim);

    ret = execvp(argv[NO_OPTS+1], argv+NO_OPTS+1);    
    
    //arbitrarily chosen to let parent know that execvp failed; we
    //reach here only if execvp fails
    return 111;
  }

  /* setting up an alarm for 'timelimit+2' seconds, since it includes CPU and I/O time */
  alarm_act.sa_handler = alarm_handler;
  sigaction(SIGALRM, &alarm_act, NULL);
  alarm(timelimit+2); 

  int status;
  FILE *fp = fopen("/tmp/setuid-helper.debug", "a");
  wait(&status);
  if(debug) 
    fprintf(fp, "submission %s status = %d\n", argv[argc-1], status);  
  if (WIFSIGNALED(status)) {
    if(debug) 
      fprintf(fp, "submission %s signalled status = %d\n", argv[argc-1], WTERMSIG(status));
    return WTERMSIG(status);
  }
  if (WIFEXITED(status)) {
    if(debug) 
      fprintf(fp, "child %s exited normally with status = %d\n", argv[argc-1], WEXITSTATUS(status));
    return WEXITSTATUS(status);
  }
  if(debug) 
    fprintf(fp, "child %s did not exit normally and did not get"
	    " signalled, exited with status = %d\n", argv[argc-1], status);
  return status;
}
