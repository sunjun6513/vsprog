/***************************************************************************
 *   Copyright (C) 2009 - 2010 by Simon Qian <SimonQian@SimonQian.com>     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "compiler.h"

#include "../versaloon_include.h"
#include "../versaloon.h"
#include "../versaloon_internal.h"
#include "usbtoxxx.h"
#include "usbtoxxx_internal.h"

vsf_err_t usbtoswd_callback(void *p, uint8_t *src, uint8_t *processed)
{
	struct versaloon_pending_t *pending = (struct versaloon_pending_t *)p;
	
	processed = processed;
	
	if (pending->extra_data != NULL)
	{
		*((uint8_t *)pending->extra_data) = src[0];
	}
	
	return VSFERR_NONE;
}

vsf_err_t usbtoswd_init(uint8_t interface_index)
{
	return usbtoxxx_init_command(USB_TO_SWD, interface_index);
}

vsf_err_t usbtoswd_fini(uint8_t interface_index)
{
	return usbtoxxx_fini_command(USB_TO_SWD, interface_index);
}

vsf_err_t usbtoswd_config(uint8_t interface_index, uint8_t trn, uint16_t retry,
					   uint16_t dly)
{
	uint8_t cfg_buf[5];
	
#if PARAM_CHECK
	if (interface_index > 7)
	{
		LOG_BUG(ERRMSG_INVALID_INTERFACE_NUM, interface_index);
		return VSFERR_FAIL;
	}
#endif
	
	cfg_buf[0] = trn;
	SET_LE_U16(&cfg_buf[1], retry);
	SET_LE_U16(&cfg_buf[3], dly);
	
	return usbtoxxx_conf_command(USB_TO_SWD, interface_index, cfg_buf, 5);
}

vsf_err_t usbtoswd_seqout(uint8_t interface_index, uint8_t *data, uint16_t bitlen)
{
	uint16_t bytelen = (bitlen + 7) >> 3;
	
#if PARAM_CHECK
	if (interface_index > 7)
	{
		LOG_BUG(ERRMSG_INVALID_INTERFACE_NUM, interface_index);
		return VSFERR_FAIL;
	}
#endif
	
	SET_LE_U16(&versaloon_cmd_buf[0], bitlen);
	memcpy(versaloon_cmd_buf + 2, data, bytelen);
	
	return usbtoxxx_out_command(USB_TO_SWD, interface_index,
								versaloon_cmd_buf, bytelen + 2, 0);
}

vsf_err_t usbtoswd_seqin(uint8_t interface_index, uint8_t *data, uint16_t bitlen)
{
	uint16_t bytelen = (bitlen + 7) >> 3;
	uint8_t buff[2];
	
#if PARAM_CHECK
	if (interface_index > 7)
	{
		LOG_BUG(ERRMSG_INVALID_INTERFACE_NUM, interface_index);
		return VSFERR_FAIL;
	}
#endif
	
	SET_LE_U16(&buff[0], bitlen);
	
	return usbtoxxx_in_command(USB_TO_SWD, interface_index, buff, 2, bytelen,
								data, 0, bytelen, 0);
}

vsf_err_t usbtoswd_transact(uint8_t interface_index, uint8_t request,
							uint32_t *data, uint8_t *ack)
{
	uint8_t buff[5];
	
#if PARAM_CHECK
	if (interface_index > 7)
	{
		LOG_BUG(ERRMSG_INVALID_INTERFACE_NUM, interface_index);
		return VSFERR_FAIL;
	}
#endif
	
	buff[0] = request;
	if (data != NULL)
	{
		SET_LE_U32(&buff[1], *data);
	}
	else
	{
		memset(buff + 1, 0, 4);
	}
	
	versaloon_set_extra_data(ack);
	versaloon_set_callback(usbtoswd_callback);
	if (request & 0x04)
	{
		// read
		return usbtoxxx_inout_command(USB_TO_SWD, interface_index, buff, 5, 5,
										(uint8_t *)data, 1, 4, 0);
	}
	else
	{
		// write
		return usbtoxxx_inout_command(USB_TO_SWD, interface_index, buff, 5, 5,
										NULL, 0, 0, 0);
	}
}

