/*
 * Copyright (c) 2017 Red Hat, Inc.
 *
 * This file is licensed to you under your choice of the GNU Lesser
 * General Public License, version 2.1 or any later version (LGPLv2.1 or
 * later), or the Apache License 2.0.
 */

#ifndef __TCMUR_CMD_HANDLER_H
#define __TCMUR_CMD_HANDLER_H

#include <stdint.h>

struct tcmu_device;
struct tcmulib_cmd;
struct tcmur_cmd;
struct timespec;

int tcmur_get_time(struct tcmu_device *dev, struct timespec *time);
int tcmur_dev_update_size(struct tcmu_device *dev, uint64_t new_size);
void tcmur_set_pending_ua(struct tcmu_device *dev, int ua);
int tcmur_generic_handle_cmd(struct tcmu_device *dev, struct tcmulib_cmd *cmd);
int tcmur_cmd_passthrough_handler(struct tcmu_device *dev, struct tcmulib_cmd *cmd);
bool tcmur_handler_is_passthrough_only(struct tcmur_handler *rhandler);
void tcmur_tcmulib_cmd_complete(struct tcmu_device *dev,
				struct tcmulib_cmd *cmd, int ret);
typedef int (*tcmur_writesame_fn_t)(struct tcmu_device *dev,
				    struct tcmur_cmd *tcmur_cmd, uint64_t off,
				    uint64_t len, struct iovec *iov,
				    size_t iov_cnt);
int tcmur_handle_writesame(struct tcmu_device *dev, struct tcmur_cmd *tcmur_cmd,
			   tcmur_writesame_fn_t write_same_fn);

typedef int (*tcmur_caw_fn_t)(struct tcmu_device *dev,
			      struct tcmur_cmd *tcmur_cmd, uint64_t off,
			      uint64_t len, struct iovec *iov, size_t iov_cnt);
int tcmur_handle_caw(struct tcmu_device *dev, struct tcmur_cmd *tcmur_cmd,
                     tcmur_caw_fn_t caw_fn);

#endif /* __TCMUR_CMD_HANDLER_H */
