/*
 * simple connect()/accept().c
 *
 * gcc ms_issue.c -o ms_issue.out -g -Wall -I<...>/install/include -L<..>/install/lib -lrt –lvma

env VMA_RX_POLL=-1 LD_PRELOAD=<...>/install/lib/libvma.so ./ms_issue.out server 4.4.4.15 7777
env VMA_RX_POLL=-1 LD_PRELOAD=<...>/install/lib/libvma.so ./ms_issue.out client 4.4.4.15 7777
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <getopt.h>
#include <assert.h>
#include <sys/select.h>
#include <sys/mman.h> /* mlock */
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <sys/epoll.h>
#include <mellanox/vma_extra.h>

#define _min(a, b) ((a) > (b) ? (b) : (a))


static inline char *_addr2str(struct sockaddr_in *addr) {
	static __thread char addrbuf[100];
	inet_ntop(AF_INET, &addr->sin_addr, addrbuf, sizeof(addrbuf));
	sprintf(addrbuf, "%s:%d", addrbuf, ntohs(addr->sin_port));

	return addrbuf;
}

int main(int argc, char *argv[]) {
	int flag;
	int fd;
	int fd_peer;
	int rc;
	socklen_t socklen;
	struct sockaddr_in addr;
	struct sockaddr in_addr;
	char *strmode = (argc > 1 ? argv[1] : NULL);
	char *straddr = (argc > 2 ? argv[2] : NULL);
	char *strport = (argc > 3 ? argv[3] : NULL);

	static struct vma_api_t *_vma_api = NULL;
	static int _vma_ring_fd = -1;

	if (!strmode || !straddr || !strport) {
		printf("Wrong options\n");
		exit(1);
	}

	printf("Mode: %s\nAddress: %s\nPort:%s\n", strmode, straddr, strport);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = PF_INET;
	addr.sin_port = htons(atoi(strport));
	addr.sin_addr.s_addr = inet_addr(straddr);

	_vma_api = vma_get_api();
	if (_vma_api == NULL) {
		printf("VMA Extra API not found\n");
	}

	fd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
	if (!fd) {
		printf("Failed to create socket\n");
		exit(1);
	}

	flag = 1;
	rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
	if (rc < 0) {
		printf("Failed to disable NAGLE\n");
		exit(1);
	}

	rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &flag, sizeof(int));
	if (rc < 0) {
		printf("Failed to setsockopt\n");
		exit(1);
	}

	if (!strcmp(strmode, "server")) {
		rc = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
		if (rc < 0) {
			printf("Failed to bind socket\n");
			exit(1);
		}

		listen(fd, 5);
		printf("Waiting on: fd=%d ip=%s\n", fd, _addr2str((struct sockaddr_in *) &addr));
	} else {
		flag = fcntl(fd, F_GETFL);
		if (flag < 0) {
			printf("failed to get socket flags\n");
		}
		flag |= O_NONBLOCK;
		rc = fcntl(fd, F_SETFL, flag);
		if (rc < 0) {
			printf("failed to set socket flags\n");
		}

		rc = connect(fd, (struct sockaddr *) &addr, sizeof(addr));
		if (rc < 0 && errno != EINPROGRESS) {
			printf("Connect failed %s\n", strerror(errno));
			exit(1);
		}
		printf("Connecting to: fd=%d ip=%s\n", fd, _addr2str((struct sockaddr_in *) &addr));
	}

	/* Need to get ring after listen() or nonblocking connect() */
	if (_vma_ring_fd < 0) {
		_vma_api->get_socket_rings_fds(fd, &_vma_ring_fd, 1);
		if (_vma_ring_fd == -1) {
			printf("Failed to return the ring fd\n");
			exit(1);
		}
	}

	rc = 0;
	while (rc == 0) {
		struct vma_completion_t vma_comps;
		rc = _vma_api->vma_poll(_vma_ring_fd, &vma_comps, 1, 0);
		if (rc > 0) {
			printf("vma_poll: rc=%d event=0x%lx user_data=%ld\n", rc, vma_comps.events, vma_comps.user_data);
			if (vma_comps.events & VMA_POLL_NEW_CONNECTION_ACCEPTED) {
				fd_peer = vma_comps.user_data;
				socklen = sizeof(in_addr);
				memcpy(&in_addr, &vma_comps.src, socklen);
				rc = 0;
				printf("Accepted connection: fd=%d from %s\n", fd_peer, _addr2str((struct sockaddr_in *) &in_addr));
				break;
			} else if (vma_comps.events & EPOLLOUT) {
				fd_peer = vma_comps.user_data;
				rc = 0;
				printf("Established connection: fd=%d\n", fd_peer);
				break;
			} else {
				printf("event=0x%lx user_data=%ld\n", vma_comps.events,
						vma_comps.user_data);
				rc = 0;
			}
		}
	}

	close(fd_peer);
	close(fd);

	return 0;
}

