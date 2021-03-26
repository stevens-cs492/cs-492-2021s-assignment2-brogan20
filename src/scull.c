#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "scull.h"

#define CDEV_NAME "/dev/scull"

/* Quantum command line option */
static int g_quantum;

static void usage(const char *cmd)
{
	printf("Usage: %s <command>\n"
	       "Commands:\n"
	       "  R          Reset quantum\n"
	       "  S <int>    Set quantum\n"
	       "  T <int>    Tell quantum\n"
	       "  G          Get quantum\n"
	       "  Q          Qeuery quantum\n"
	       "  X <int>    Exchange quantum\n"
	       "  H <int>    Shift quantum\n"
	       "  h          Print this message\n",
	       cmd);
}

typedef int cmd_t;

static cmd_t parse_arguments(int argc, const char **argv)
{
	cmd_t cmd;

	if (argc < 2) {
		fprintf(stderr, "%s: Invalid number of arguments\n", argv[0]);
		cmd = -1;
		goto ret;
	}

	/* Parse command and optional int argument */
	cmd = argv[1][0];
	switch (cmd) {
	case 'S':
	case 'T':
	case 'H':
	case 'p':
	case 't':
	case 'X':
		if (argc < 3) {
			fprintf(stderr, "%s: Missing quantum\n", argv[0]);
			cmd = -1;
			break;
		}
		g_quantum = atoi(argv[2]);
		break;
	case 'R':
	case 'G':
	case 'k':
	case 'Q':
	case 'h':
		break;
	default:
		fprintf(stderr, "%s: Invalid command\n", argv[0]);
		cmd = -1;
	}

ret:
	if (cmd < 0 || cmd == 'h') {
		usage(argv[0]);
		exit((cmd == 'h')? EXIT_SUCCESS : EXIT_FAILURE);
	}
	return cmd;
}



static int do_op(int fd, cmd_t cmd)
{
	int ret, q;
	pthread_t pids[10];
	struct task_info t;

	switch (cmd) {
	case 'k':
		ret = ioctl(fd, SCULL_IOCKQUANTUM, &t);
		printf("state %lu, stack %lx, cpu %u, prio %d, sprio %d, nprio %d, rtprio %u, pid %d, tgid %d, nv %ul, niv %ul\n", t.state, (unsigned long)t.stack, t.cpu, t.prio, t.static_prio, t.normal_prio, t.rt_priority, t.pid, t.tgid, t.nvcsw, t.nivcsw);

		break;
	case 'R':
		ret = ioctl(fd, SCULL_IOCRESET);
		if (ret == 0)
			printf("Quantum reset\n");
		break;
	case 'Q':
		q = ioctl(fd, SCULL_IOCQQUANTUM);
		printf("Quantum: %d\n", q);
		ret = 0;
		break;
	case 'G':
		ret = ioctl(fd, SCULL_IOCGQUANTUM, &q);
		if (ret == 0)
			printf("Quantum: %d\n", q);
		break;
	case 'T':
		ret = ioctl(fd, SCULL_IOCTQUANTUM, g_quantum);
		if (ret == 0)
			printf("Quantum set\n");
		break;
	case 'S':
		q = g_quantum;
		ret = ioctl(fd, SCULL_IOCSQUANTUM, &q);
		if (ret == 0)
			printf("Quantum set\n");
		break;
	case 'X':
		q = g_quantum;
		ret = ioctl(fd, SCULL_IOCXQUANTUM, &q);
		if (ret == 0)
			printf("Quantum exchanged, old quantum: %d\n", q);
		break;
	case 'H':
		q = ioctl(fd, SCULL_IOCHQUANTUM, g_quantum);
		printf("Quantum shifted, old quantum: %d\n", q);
		ret = 0;
		break;
	case 't':
		/*
		// Check the bounds are right
		if (g_quantum < 1 || g_quantum > 10) {
			ret = -1;
			break;
		}
		// Allocate memory for the args
		if ((a = (struct args*)malloc(g_quantum * sizeof(a))) == NULL) {
			ret = -1;
			printf("Cannot allocate thread args");
			break;
		}
		// Run threads
		for (i = 0; i < g_quantum; i++) {
			a[i].fd = fd;
			a[i].ti = t[i];
			if (pthread_create(&pids[i], NULL, &func, (void *)&a[i]) != 0) {
				ret = -1;
				goto err;
			}
		}
		// Finish threads
		for (i = 0; i < g_quantum; i++) {
			if (pthread_join(&pids[i], NULL) != 0) {
				ret = -1;
				goto err;
			}
			printf("state %l, stack %lx, cpu %u, prio %d, sprio %d, nprio %d, rtprio %u, pid %d, tgid %d, nv %ul, niv %ul", t[i]->state, (unsigned long)t[i]->stack, t[i]->cpu, t[i]->prio, t[i]->static_prio, t[i]->normal_prio, t[i]->rt_priority, t[i]->pid, t[i]->tgid, t[i]->nvcsw, t[i]->nivcsw);
		}
		*/

		break;
	default:
		/* Should never occur */
		abort();
		ret = -1; /* Keep the compiler happy */
	}
err:
	if (ret != 0)
		perror("ioctl");
	return ret;
}

int main(int argc, const char **argv)
{
	int fd, ret;
	cmd_t cmd;

	cmd = parse_arguments(argc, argv);

	fd = open(CDEV_NAME, O_RDONLY);
	if (fd < 0) {
		perror("cdev open");
		return EXIT_FAILURE;
	}

	printf("Device (%s) opened\n", CDEV_NAME);

	ret = do_op(fd, cmd);

	if (close(fd) != 0) {
		perror("cdev close");
		return EXIT_FAILURE;
	}

	printf("Device (%s) closed\n", CDEV_NAME);

	return (ret != 0)? EXIT_FAILURE : EXIT_SUCCESS;
}
