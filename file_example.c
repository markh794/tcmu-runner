/*
 * Copyright (c) 2014 Red Hat, Inc.
 *
 * This file is licensed to you under your choice of the GNU Lesser
 * General Public License, version 2.1 or any later version (LGPLv2.1 or
 * later), or the Apache License 2.0.
 */

/*
 * Example code to demonstrate how a TCMU handler might work.
 *
 * Using the example of backing a device by a file to demonstrate:
 *
 * 1) Registering with tcmu-runner
 * 2) Parsing the handler-specific config string as needed for setup
 * 3) Opening resources as needed
 * 4) Handling SCSI commands and using the handler API
 */

#define _GNU_SOURCE
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <endian.h>
#include <errno.h>
#include <scsi/scsi.h>

#include "scsi_defs.h"
#include "libtcmu.h"
#include "tcmu-runner.h"
#include "tcmur_device.h"

struct file_state {
	int fd;
};

static int file_open(struct tcmu_device *dev, bool reopen)
{
	struct file_state *state;
	char *config;

	state = calloc(1, sizeof(*state));
	if (!state)
		return -ENOMEM;

	tcmur_dev_set_private(dev, state);

	config = strchr(tcmu_dev_get_cfgstring(dev), '/');
	if (!config) {
		tcmu_err("no configuration found in cfgstring\n");
		goto err;
	}
	config += 1; /* get past '/' */

	tcmu_dev_set_write_cache_enabled(dev, 1);

	state->fd = open(config, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (state->fd == -1) {
		tcmu_err("could not open %s: %m\n", config);
		goto err;
	}

	tcmu_dbg("config %s\n", tcmu_dev_get_cfgstring(dev));

	return 0;

err:
	free(state);
	return -EINVAL;
}

static void file_close(struct tcmu_device *dev)
{
	struct file_state *state = tcmur_dev_get_private(dev);

	close(state->fd);
	free(state);
}

static int file_read(struct tcmu_device *dev, struct tcmur_cmd *cmd,
		     struct iovec *iov, size_t iov_cnt, size_t length,
		     off_t offset)
{
	struct file_state *state = tcmur_dev_get_private(dev);
	size_t remaining = length;
	ssize_t ret;

	while (remaining) {
		ret = preadv(state->fd, iov, iov_cnt, offset);
		if (ret < 0) {
			tcmu_err("read failed: %m\n");
			ret = TCMU_STS_RD_ERR;
			goto done;
		}

		if (ret == 0) {
			/* EOF, then zeros the iovecs left */
			tcmu_iovec_zero(iov, iov_cnt);
			break;
		}

		tcmu_iovec_seek(iov, ret);
		offset += ret;
		remaining -= ret;
	}
	ret = TCMU_STS_OK;
done:
	return ret;
}

static int file_write(struct tcmu_device *dev, struct tcmur_cmd *cmd,
		      struct iovec *iov, size_t iov_cnt, size_t length,
		      off_t offset)
{
	struct file_state *state = tcmur_dev_get_private(dev);
	size_t remaining = length;
	ssize_t ret;

	while (remaining) {
		ret = pwritev(state->fd, iov, iov_cnt, offset);
		if (ret < 0) {
			tcmu_err("write failed: %m\n");
			ret = TCMU_STS_WR_ERR;
			goto done;
		}
		tcmu_iovec_seek(iov, ret);
		offset += ret;
		remaining -= ret;
	}
	ret = TCMU_STS_OK;
done:
	return ret;
}

static int file_flush(struct tcmu_device *dev, struct tcmur_cmd *cmd)
{
	struct file_state *state = tcmur_dev_get_private(dev);
	int ret;

	if (fsync(state->fd)) {
		tcmu_err("sync failed\n");
		ret = TCMU_STS_WR_ERR;
		goto done;
	}
	ret = TCMU_STS_OK;
done:
	return ret;
}

static int file_reconfig(struct tcmu_device *dev, struct tcmulib_cfg_info *cfg)
{
	switch (cfg->type) {
	case TCMULIB_CFG_DEV_SIZE:
		/*
		 * TODO - For open/reconfig we should make sure the FS the
		 * file is on is large enough for the requested size. For
		 * now assume we can grow the file and return 0.
		 */
		return 0;
	case TCMULIB_CFG_DEV_CFGSTR:
	case TCMULIB_CFG_WRITE_CACHE:
	default:
		return -EOPNOTSUPP;
	}
}

static const char file_cfg_desc[] =
	"The path to the file to use as a backstore.";

static struct tcmur_handler file_handler = {
	.cfg_desc = file_cfg_desc,

	.reconfig = file_reconfig,

	.open = file_open,
	.close = file_close,
	.read = file_read,
	.write = file_write,
	.flush = file_flush,
	.name = "File-backed Handler (example code)",
	.subtype = "file",
	.nr_threads = 2,
};

/* Entry point must be named "handler_init". */
int handler_init(void)
{
	return tcmur_register_handler(&file_handler);
}
