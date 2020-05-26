#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>

#define DSIZE 4096 // Size of data that can be atomically written to named pipe

struct stringbuf
{
    char buf[DSIZE];
    char *cur;
    char *end;
};

static void stringbuf_init(struct stringbuf *s)
{
    s->cur = s->buf;
    s->end = s->buf + DSIZE - 1;
}

static void stringbuf_append(struct stringbuf *sbuf, const char *str)
{
    const char *p = str;
    while (*p && sbuf->cur < sbuf->end)
	*(sbuf->cur++) = *p++;
}

static void stringbuf_append_int(struct stringbuf *sbuf, int v)
{
    char x[10];
    snprintf(x, sizeof(x), "%d", v);
    stringbuf_append(sbuf, x);
}

static void stringbuf_append_ulong(struct stringbuf *sbuf, unsigned long v)
{
    char x[32];
    snprintf(x, sizeof(x), "%lu", v);
    stringbuf_append(sbuf, x);
}

static void send_output(struct stringbuf *sbuf)
{
    char *fifo = getenv("P3_SHEPHERD_FIFO");
    if (fifo)
    {
	int fd = open(fifo, O_WRONLY);
	if (fd >= 0)
	{
	    int n = write(fd, sbuf->buf, sbuf->cur - sbuf->buf);
	    close(fd);
	}
    }
}    

static void stringbuf_append_argv(struct stringbuf *sbuf, char *const argv[])
{
    char * const *arg = argv;
    while (*arg)
    {
	stringbuf_append(sbuf, *arg);
	stringbuf_append(sbuf, "\n");
	arg++;
    }
}

static void stringbuf_append_argc_argv(struct stringbuf *sbuf, int argc, char *const argv[])
{
    char * const *arg = argv;
    int i;
    for (i = 0; i < argc; i++)
    {
	stringbuf_append(sbuf, argv[i]);
	stringbuf_append(sbuf, "\n");
	arg++;
    }
}


int execve(const char *progname, char *const argv[], char *const envp[]) {
    int		(*original_execve)(const char *, char *const[], char *const[]);

    /* data packet is START\tprogname\nargc\narg...\n */

    struct stringbuf dbuf;
    stringbuf_init(&dbuf);
    stringbuf_append(&dbuf, "execve\n");
    stringbuf_append_int(&dbuf, getpid());
    stringbuf_append(&dbuf, "\n");
    stringbuf_append(&dbuf, progname);
    stringbuf_append(&dbuf, "\n");
    stringbuf_append_argv(&dbuf, argv);
    stringbuf_append(&dbuf, "\0");
    send_output(&dbuf);

    *(void **)(&original_execve) = dlsym(RTLD_NEXT, "execve");  
    return((*original_execve)(progname, argv, envp));
}

static void send_usage(char *who)
{
    char sbuf[1024];
    *sbuf = 0;
    FILE *fp = fopen("/proc/self/stat", "r");
    if (fp)
    {
	fgets(sbuf, sizeof(sbuf), fp);
	fclose(fp);
    }
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) == 0)
    {
	struct stringbuf dbuf;
	stringbuf_init(&dbuf);
	stringbuf_append(&dbuf, who);
	stringbuf_append(&dbuf, "\n");
	stringbuf_append_int(&dbuf, getpid());
	stringbuf_append(&dbuf, "\n");
	stringbuf_append(&dbuf, sbuf);
	stringbuf_append_ulong(&dbuf, ru.ru_utime.tv_sec);
	stringbuf_append(&dbuf, "\n");
	stringbuf_append_ulong(&dbuf, ru.ru_utime.tv_usec);
	stringbuf_append(&dbuf, "\n");
	stringbuf_append_ulong(&dbuf, ru.ru_stime.tv_sec);
	stringbuf_append(&dbuf, "\n");
	stringbuf_append_ulong(&dbuf, ru.ru_stime.tv_usec);
	stringbuf_append(&dbuf, "\n\0");

	send_output(&dbuf);
    }
}

void exit(int status)
{
    int		(*original_exit)(int);
    send_usage("exit");
    *(void **)(&original_exit) = dlsym(RTLD_NEXT, "exit");  
    (*original_exit)(status);
}

__attribute__((constructor)) void _p3x_preload_init(int argc, char *const argv[], char *const envp[])
{
    char b[100];
    *b = 0;
    FILE *fp = fopen("/proc/self/stat", "r");
    if (fp)
    {
	fscanf(fp, b, "%d");
	fclose(fp);
    }
    
    struct stringbuf dbuf;
    stringbuf_init(&dbuf);
    stringbuf_append(&dbuf, "START\n");
    stringbuf_append_int(&dbuf, getpid());
    stringbuf_append(&dbuf, "\n");
    stringbuf_append(&dbuf, b);
    stringbuf_append(&dbuf, "\n");
    stringbuf_append_argc_argv(&dbuf, argc, argv);
    stringbuf_append(&dbuf, "\0");
    send_output(&dbuf);
}
__attribute__((destructor)) void done()
{
    send_usage("done");
}
