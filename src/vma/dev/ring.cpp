/*
 * Copyright (C) Mellanox Technologies Ltd. 2001-2013.  ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of Mellanox Technologies Ltd.
 * (the "Company") and all right, title, and interest in and to the software product,
 * including all associated intellectual property rights, are and shall
 * remain exclusively with the Company.
 *
 * This software is made available under either the GPL v2 license or a commercial license.
 * If you wish to obtain a commercial license, please contact Mellanox at support@mellanox.com.
 */


#include "ring.h"

ring::ring(int count, uint32_t mtu) :
	m_n_num_resources(count), m_p_n_rx_channel_fds(NULL), m_parent(NULL),
	m_vma_active(false), m_mtu(mtu)
{
}

ring::~ring()
{
	while (!m_ec_queue.empty()) {
		m_ec_queue.pop_front();
	}
}
