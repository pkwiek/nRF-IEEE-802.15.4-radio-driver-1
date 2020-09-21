/* Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 *   This file implements hooks for the 802.15.4 driver Core module.
 *
 * Hooks are used by optional driver features to modify way in which notifications are propagated
 * through the driver.
 *
 */

#include "nrf_802154_core_hooks.h"

#include <stdbool.h>

#include "mac_features/nrf_802154_ack_timeout.h"
#include "mac_features/nrf_802154_csma_ca.h"
#include "mac_features/nrf_802154_delayed_trx.h"
#include "mac_features/nrf_802154_ifs.h"
#include "mac_features/nrf_802154_tx_timeout.h"
#include "nrf_802154_config.h"
#include "nrf_802154_types.h"

typedef bool (* abort_hook)(nrf_802154_term_t term_lvl, req_originator_t req_orig);
typedef void (* transmission_ready_hook)(const uint8_t * p_frame, bool ready);
typedef void (* transmitted_hook)(const uint8_t * p_frame);
typedef bool (* tx_failed_hook)(const uint8_t * p_frame, nrf_802154_tx_error_t error);
typedef bool (* tx_started_hook)(const uint8_t * p_frame);
typedef void (* rx_started_hook)(const uint8_t * p_frame);
typedef void (* rx_ack_started_hook)(void);
typedef void (* prio_changed_hook)(uint32_t old_prio, uint32_t new_prio);
typedef bool (* pre_transmission_hook)(const uint8_t                     * p_frame,
                                       bool                                cca,
                                       nrf_802154_transmit_failed_notify_t notify_function);

/* Since some compilers do not allow empty initializers for arrays with unspecified bounds,
 * NULL pointer is appended to below arrays if the compiler used is not GCC. It is intentionally
 * unused, but it prevents the arrays from being empty. GCC manages to optimize empty arrays away,
 * so such a solution is unnecessary. */

static const abort_hook m_abort_hooks[] =
{
#if NRF_802154_CSMA_CA_ENABLED
    nrf_802154_csma_ca_abort,
#endif

#if NRF_802154_ACK_TIMEOUT_ENABLED
    nrf_802154_ack_timeout_abort,
#endif

#if NRF_802154_DELAYED_TRX_ENABLED
    nrf_802154_delayed_trx_abort,
#endif

#if NRF_802154_IFS_ENABLED
    nrf_802154_ifs_abort,
#endif

    nrf_802154_tx_timeout_abort,
};

static const pre_transmission_hook m_pre_transmission_hooks[] =
{
#if NRF_802154_CSMA_CA_ENABLED
    nrf_802154_csma_ca_pretransmission,
#endif
#if NRF_802154_IFS_ENABLED
    nrf_802154_ifs_pretransmission,
#endif

    NULL,
};

static const transmission_ready_hook m_transmission_ready_hooks[] = 
{
    nrf_802154_tx_timeout_transmission_ready
};

static const transmitted_hook m_transmitted_hooks[] =
{
#if NRF_802154_ACK_TIMEOUT_ENABLED
    nrf_802154_ack_timeout_transmitted_hook,
#endif
#if NRF_802154_IFS_ENABLED
    nrf_802154_ifs_transmitted_hook,
#endif
    NULL,
};

static const tx_failed_hook m_tx_failed_hooks[] =
{
#if NRF_802154_CSMA_CA_ENABLED
    nrf_802154_csma_ca_tx_failed_hook,
#endif

#if NRF_802154_ACK_TIMEOUT_ENABLED
    nrf_802154_ack_timeout_tx_failed_hook,
#endif

    NULL,
};

static const tx_started_hook m_tx_started_hooks[] =
{
#if NRF_802154_CSMA_CA_ENABLED
    nrf_802154_csma_ca_tx_started_hook,
#endif

#if NRF_802154_ACK_TIMEOUT_ENABLED
    nrf_802154_ack_timeout_tx_started_hook,
#endif

    NULL,
};

static const rx_started_hook m_rx_started_hooks[] =
{
#if NRF_802154_DELAYED_TRX_ENABLED
    nrf_802154_delayed_trx_rx_started_hook,
#endif

    NULL,
};

static const rx_ack_started_hook m_rx_ack_started_hooks[] =
{
#if NRF_802154_ACK_TIMEOUT_ENABLED
    nrf_802154_ack_timeout_rx_ack_started_hook,
#endif

    NULL,
};

static const prio_changed_hook m_prio_changed_hooks[] =
{
#if NRF_802154_CSMA_CA_ENABLED
    nrf_802154_csma_ca_prio_changed_hook,
#endif

    NULL,
};

bool nrf_802154_core_hooks_terminate(nrf_802154_term_t term_lvl, req_originator_t req_orig)
{
    bool result = true;

    for (uint32_t i = 0; i < sizeof(m_abort_hooks) / sizeof(m_abort_hooks[0]); i++)
    {
        if (m_abort_hooks[i] == NULL)
        {
            break;
        }

        result = m_abort_hooks[i](term_lvl, req_orig);

        if (!result)
        {
            break;
        }
    }

    return result;
}

bool nrf_802154_core_hooks_pre_transmission(const uint8_t                     * p_frame,
                                            bool                                cca,
                                            nrf_802154_transmit_failed_notify_t notify_function)
{
    bool result = true;

    for (uint32_t i = 0; i < sizeof(m_pre_transmission_hooks) / sizeof(m_pre_transmission_hooks[0]);
         i++)
    {
        if (m_pre_transmission_hooks[i] == NULL)
        {
            break;
        }

        result = m_pre_transmission_hooks[i](p_frame, cca, notify_function);

        if (!result)
        {
            break;
        }
    }

    return result;
}

void nrf_802154_core_hooks_transmission_ready(const uint8_t * p_frame, bool ready)
{
    uint32_t nb_hooks = sizeof(m_transmission_ready_hooks) / sizeof(m_transmission_ready_hooks[0]);

    for (uint32_t i = 0; i < nb_hooks; i++)
    {
        if (m_transmission_ready_hooks[i] == NULL)
        {
            break;
        }

        m_transmission_ready_hooks[i](p_frame, ready);
    }
}

void nrf_802154_core_hooks_transmitted(const uint8_t * p_frame)
{
    for (uint32_t i = 0; i < sizeof(m_transmitted_hooks) / sizeof(m_transmitted_hooks[0]); i++)
    {
        if (m_transmitted_hooks[i] == NULL)
        {
            break;
        }

        m_transmitted_hooks[i](p_frame);
    }
}

bool nrf_802154_core_hooks_tx_failed(const uint8_t * p_frame, nrf_802154_tx_error_t error)
{
    bool result = true;

    for (uint32_t i = 0; i < sizeof(m_tx_failed_hooks) / sizeof(m_tx_failed_hooks[0]); i++)
    {
        if (m_tx_failed_hooks[i] == NULL)
        {
            break;
        }

        result = m_tx_failed_hooks[i](p_frame, error);

        if (!result)
        {
            break;
        }
    }

    return result;
}

bool nrf_802154_core_hooks_tx_started(const uint8_t * p_frame)
{
    bool result = true;

    for (uint32_t i = 0; i < sizeof(m_tx_started_hooks) / sizeof(m_tx_started_hooks[0]); i++)
    {
        if (m_tx_started_hooks[i] == NULL)
        {
            break;
        }

        result = m_tx_started_hooks[i](p_frame);

        if (!result)
        {
            break;
        }
    }

    return result;
}

void nrf_802154_core_hooks_rx_started(const uint8_t * p_frame)
{
    for (uint32_t i = 0; i < sizeof(m_rx_started_hooks) / sizeof(m_rx_started_hooks[0]); i++)
    {
        if (m_rx_started_hooks[i] == NULL)
        {
            break;
        }

        m_rx_started_hooks[i](p_frame);
    }
}

void nrf_802154_core_hooks_rx_ack_started(void)
{
    for (uint32_t i = 0; i < sizeof(m_rx_ack_started_hooks) / sizeof(m_rx_ack_started_hooks[0]);
         i++)
    {
        if (m_rx_ack_started_hooks[i] == NULL)
        {
            break;
        }

        m_rx_ack_started_hooks[i]();
    }
}

void nrf_802154_core_hooks_prio_changed(uint32_t old_prio, uint32_t new_prio)
{
    for (uint32_t i = 0; i < sizeof(m_prio_changed_hooks) / sizeof(m_prio_changed_hooks[0]); i++)
    {
        if (m_prio_changed_hooks[i] == NULL)
        {
            break;
        }

        m_prio_changed_hooks[i](old_prio, new_prio);
    }
}
