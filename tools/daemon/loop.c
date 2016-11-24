/*
 * Copyright (c) 2016 Mellanox Technologies, Ltd. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "vma/util/agent.h"
#include "hash.h"
#include "daemon.h"

extern int open_store(void);
extern void close_store(void);
extern int open_message(void);
extern void close_message(void);
extern int proc_message(void);
extern int open_notify(void);
extern void close_notify(void);
extern int proc_notify(void);

int proc_loop(void)
{
	int rc = 0;

	log_debug("setting working directory ...\n");
	if ((mkdir(VMA_AGENT_PATH, 0777) != 0) && (errno != EEXIST)) {
		rc = -errno;
		log_error("failed create folder %s (errno = %d)\n", VMA_AGENT_PATH, errno);
		goto err;
	}

	log_debug("setting store ...\n");
	rc = open_store();
	if (rc < 0) {
		goto err;
	}

	log_debug("setting notification ...\n");
	rc = open_notify();
	if (rc < 0) {
		goto err;
	}

	log_debug("setting message processing ...\n");
	rc = open_message();
	if (rc < 0) {
		goto err;
	}

	log_debug("starting loop ...\n");
	while ((0 == daemon_cfg.sig) && (0 == rc)) {
		fd_set readfds;
		struct timeval tv;
		int max_fd = -1;

		FD_ZERO(&readfds);
		FD_SET(daemon_cfg.sock_fd, &readfds);
		max_fd = daemon_cfg.sock_fd;
		FD_SET(daemon_cfg.notify_fd, &readfds);
		max_fd = (max_fd < daemon_cfg.notify_fd ? daemon_cfg.notify_fd : max_fd);

		/* Use timeout for select() call */
		tv.tv_sec = 60;
		tv.tv_usec = 0;

		rc = select(max_fd + 1, &readfds, NULL, NULL, &tv);
		if (rc < 0) {
			if (errno != EINTR) {
				log_error("Failed select() errno %d (%s)\n", errno,
						strerror(errno));
			}
			rc = -errno;
			goto err;
		} else if (rc == 0) {
			continue;
		}

		/* Check any events from fanotify */
		if (FD_ISSET(daemon_cfg.notify_fd, &readfds)) {
			log_debug("notification processing ...\n");
			rc = proc_notify();
		}

		/* Check messages from processes */
		if (FD_ISSET(daemon_cfg.sock_fd, &readfds)) {
			log_debug("message processing ...\n");
			rc = proc_message();
		}
	}

err:
	log_debug("finishing loop ...\n");

	close_store();

	close_message();

	close_notify();

	return rc;
}
