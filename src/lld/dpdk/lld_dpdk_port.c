/*
 * Copyright(c) 2021 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
 * @file lld_dpdk_port.c
 *
 *
 * Definitions for LLD DPDK APIs.
 */

#include "lld_dpdk_port.h"
#include "../lldlib_log.h"

int lld_dpdk_tap_port_create(port_attributes_t *port_attrib)
{
	if (!tap_create(port_attrib->port_name)) {
		LOG_ERROR("Creation of Tap Port %s failed\n",
			  port_attrib->port_name);
		return BF_INVALID_ARG;
	}

	return BF_SUCCESS;
}

int lld_dpdk_pipeline_tap_port_add(bf_dev_port_t dev_port,
				   port_attributes_t *port_attrib,
				   struct pipeline *pipe,
				   struct mempool *mp)
{
	struct rte_swx_port_fd_reader_params params_in;
	struct rte_swx_port_fd_writer_params params_out;
	struct tap *tap = NULL;
	int status;

	memset(&params_in, 0, sizeof(params_in));
	memset(&params_out, 0, sizeof(params_out));

	tap = tap_find(port_attrib->port_name);
	if (!tap) {
		LOG_ERROR("Tap Port %s not found\n", port_attrib->port_name);
		return BF_INVALID_ARG;
	}

	if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
	    (port_attrib->port_dir == PM_PORT_DIR_RX_ONLY)) {
		params_in.fd = tap->fd;
		params_in.mempool = mp->m;
		params_in.mtu = port_attrib->tap.mtu;
		params_in.burst_size = 1;

		status = rte_swx_pipeline_port_in_config(pipe->p,
							port_attrib->port_in_id,
							"fd",
							&params_in);

		if (status) {
			LOG_ERROR("Port in Error for Tap Port %s\n",
				  port_attrib->port_name);
			return BF_INVALID_ARG;
		}
	}

	if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
	    (port_attrib->port_dir == PM_PORT_DIR_TX_ONLY)) {
		params_out.fd = tap->fd;
		params_out.burst_size = 1;

		status = rte_swx_pipeline_port_out_config(pipe->p,
						port_attrib->port_out_id,
						"fd",
						&params_out);

		if (status) {
			LOG_ERROR("Port out Error for Tap Port %s\n",
				  port_attrib->port_name);
			return BF_INVALID_ARG;
		}
	}

	return BF_SUCCESS;
}

int lld_dpdk_tap_port_add(bf_dev_port_t dev_port,
			  port_attributes_t *port_attrib,
			  struct pipeline *pipe,
			  struct mempool *mp)
{
	if (lld_dpdk_tap_port_create(port_attrib) != BF_SUCCESS)
		return BF_INVALID_ARG;

	if (lld_dpdk_pipeline_tap_port_add(dev_port, port_attrib, pipe, mp)
	    != BF_SUCCESS)
		return BF_INVALID_ARG;

	return BF_SUCCESS;
}

int lld_dpdk_link_port_create(port_attributes_t *port_attrib)
{
	struct link_params p;

	memset(&p, 0, sizeof(p));

	p.dev_name = port_attrib->link.pcie_domain_bdf;
	p.dev_args = port_attrib->link.dev_args;
	p.dev_hotplug_enabled = port_attrib->link.dev_hotplug_enabled;
	p.rx.mempool_name = port_attrib->mempool_name;
	p.rx.n_queues = 1;
	p.rx.queue_size = 128;
	p.tx.n_queues = 1;
	p.tx.queue_size = 512;
	p.promiscuous = 1;
	p.rx.rss = NULL;

	if (!link_create(port_attrib->port_name, &p)) {
		LOG_ERROR("Creation of Link Port %s failed\n",
			  port_attrib->port_name);
		return BF_INVALID_ARG;
	}

	return BF_SUCCESS;
}

int lld_dpdk_pipeline_link_port_add(bf_dev_port_t dev_port,
				    port_attributes_t *port_attrib,
				    struct pipeline *pipe)
{
	struct rte_swx_port_ethdev_reader_params params_in;
	struct rte_swx_port_ethdev_writer_params params_out;
	struct link *link = NULL;
	int status;

	memset(&params_in, 0, sizeof(params_in));
	memset(&params_out, 0, sizeof(params_out));

	link = link_find(port_attrib->port_name);
	if (!link) {
		LOG_ERROR("Link Port %s not found\n", port_attrib->port_name);
		return BF_INVALID_ARG;
	}

	if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
	    (port_attrib->port_dir == PM_PORT_DIR_RX_ONLY)) {
		params_in.dev_name = link->dev_name;
		params_in.queue_id = 0;
		params_in.burst_size = 1;

		status = rte_swx_pipeline_port_in_config(pipe->p,
						 port_attrib->port_in_id,
						 "ethdev",
						 &params_in);

		if (status) {
			LOG_ERROR("Port in Error for Link Port %s\n",
				  port_attrib->port_name);
			return BF_INVALID_ARG;
		}
	}

	if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
	    (port_attrib->port_dir == PM_PORT_DIR_TX_ONLY)) {
		params_out.dev_name = link->dev_name;
		params_out.queue_id = 0;
		params_out.burst_size = 1;

		status = rte_swx_pipeline_port_out_config(pipe->p,
						  port_attrib->port_out_id,
						  "ethdev",
						  &params_out);

		if (status) {
			LOG_ERROR("Port out Error for Link Port %s\n",
				  port_attrib->port_name);
			return BF_INVALID_ARG;
		}
	}

	return BF_SUCCESS;
}

int lld_dpdk_link_port_add(bf_dev_port_t dev_port,
			   port_attributes_t *port_attrib,
			   struct pipeline *pipe)
{
	if (lld_dpdk_link_port_create(port_attrib) != BF_SUCCESS)
		return BF_INVALID_ARG;

	if (lld_dpdk_pipeline_link_port_add(dev_port, port_attrib, pipe)
	    != BF_SUCCESS)
		return BF_INVALID_ARG;

	return BF_SUCCESS;
}

int lld_dpdk_source_port_add(bf_dev_port_t dev_port,
			     port_attributes_t *port_attrib,
			     struct pipeline *pipe,
			     struct mempool *mp)
{
	struct rte_swx_port_source_params params;
	int status;

	memset(&params, 0, sizeof(params));

	params.pool = mp->m;
	params.file_name = port_attrib->source.file_name;

	status = rte_swx_pipeline_port_in_config(pipe->p,
						port_attrib->port_in_id,
						"source",
						&params);

	if (status) {
		LOG_ERROR("Creation of Source Port %s failed\n",
			  port_attrib->port_name);
		return BF_INVALID_ARG;
	}

	return BF_SUCCESS;
}

int lld_dpdk_sink_port_add(bf_dev_port_t dev_port,
			port_attributes_t *port_attrib,
			struct pipeline *pipe)
{
	struct rte_swx_port_sink_params params;
	int status;

	memset(&params, 0, sizeof(params));

	if (!strcmp(port_attrib->sink.file_name, "none"))
		params.file_name = NULL;
	else
		params.file_name = port_attrib->sink.file_name;

	status = rte_swx_pipeline_port_out_config(pipe->p,
						  port_attrib->port_out_id,
						  "sink",
						  &params);

	if (status) {
		LOG_ERROR("Creation of Sink Port %s failed\n",
			  port_attrib->port_name);
		return BF_INVALID_ARG;
	}

	return BF_SUCCESS;
}

int lld_dpdk_port_stats_get(bf_dev_port_t dev_port,
			port_attributes_t *port_attrib,
			uint64_t *stats)
{
	struct pipeline *pipe = NULL;
	struct rte_swx_port_in_stats in_stats;
	struct rte_swx_port_out_stats out_stats;
	int status;

	memset(&in_stats, 0, sizeof(in_stats));
	memset(&out_stats, 0, sizeof(out_stats));

	pipe = pipeline_find(port_attrib->pipe_name);
	if (!pipe || !pipe->ctl) {
		LOG_ERROR("Pipeline %s is not valid\n", port_attrib->pipe_name);
		return BF_INVALID_ARG;
	}

	if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
	    (port_attrib->port_dir == PM_PORT_DIR_RX_ONLY)) {
		status = rte_swx_ctl_pipeline_port_in_stats_read(pipe->p,
						    port_attrib->port_in_id,
						    &in_stats);

		if (status) {
			LOG_ERROR("Failed to Read Input Stats for Port %s\n",
				  port_attrib->port_name);
			return BF_INVALID_ARG;
		}

		stats[INPUT_PACKETS] = in_stats.n_pkts;
		stats[INPUT_BYTES] = in_stats.n_bytes;
		stats[INPUT_EMPTY_POLLS] = in_stats.n_empty;
	}

	if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
	    (port_attrib->port_dir == PM_PORT_DIR_TX_ONLY)) {
		status = rte_swx_ctl_pipeline_port_out_stats_read(pipe->p,
						    port_attrib->port_out_id,
						    &out_stats);

		if (status) {
			LOG_ERROR("Failed to Read Output Stats for Port %s\n",
				  port_attrib->port_name);
			return BF_INVALID_ARG;
		}

		stats[OUTPUT_PACKETS] = out_stats.n_pkts;
		stats[OUTPUT_BYTES] = out_stats.n_bytes;
	}

	return BF_SUCCESS;
}

int lld_dpdk_port_add(bf_dev_port_t dev_port,
		      port_attributes_t *port_attrib)
{
	int status = BF_SUCCESS;
	struct pipeline *pipe = NULL;
	struct mempool *mp = NULL;

	pipe = pipeline_find(port_attrib->pipe_name);
	if (!pipe || pipe->ctl) {
		LOG_ERROR("Pipeline %s is not valid\n", port_attrib->pipe_name);
		return BF_INVALID_ARG;
	}

	if (port_attrib->port_type != BF_DPDK_SINK) {
		mp = mempool_find(port_attrib->mempool_name);
		if (!mp) {
			LOG_ERROR("Mempool %s not found\n",
				  port_attrib->mempool_name);
			return BF_INVALID_ARG;
		}
	}

	switch (port_attrib->port_type) {
	case BF_DPDK_TAP:
	{
		status = lld_dpdk_tap_port_add(dev_port, port_attrib, pipe, mp);
		break;
	}
	case BF_DPDK_LINK:
	{
		status = lld_dpdk_link_port_add(dev_port, port_attrib, pipe);
		break;
	}
	case BF_DPDK_SOURCE:
	{
		status = lld_dpdk_source_port_add(dev_port, port_attrib, pipe,
						  mp);
		break;
	}
	case BF_DPDK_SINK:
	{
		status = lld_dpdk_sink_port_add(dev_port, port_attrib, pipe);
		break;
	}
	default:
		LOG_TRACE("%s:%d Incorrect Port Type", __func__, __LINE__);
		status = BF_INVALID_ARG;
		break;
	}

	return status;
}
