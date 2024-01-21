/* vim:set ft=c ts=2 sw=2 sts=2 et cindent: */
/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MIT
 *
 * Portions created by Alan Antonuk are Copyright (c) 2012-2013
 * Alan Antonuk. All Rights Reserved.
 *
 * Portions created by VMware are Copyright (c) 2007-2012 VMware, Inc.
 * All Rights Reserved.
 *
 * Portions created by Tony Garnock-Jones are Copyright (c) 2009-2010
 * VMware, Inc. and Tony Garnock-Jones. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ***** END LICENSE BLOCK *****
 */

#include <amqp.h>
#include <amqp_framing.h>
#include <amqp_tcp_socket.h>
#include <spot/utility/Logging.h>
#include "MqUtils.h"

bool die_on_amqp_error(amqp_rpc_reply_t x, char const *context)
{
  switch (x.reply_type) {
  case AMQP_RESPONSE_NORMAL:
    return false;

  case AMQP_RESPONSE_NONE:
    LOG_ERROR << context << "missing RPC reply type!";
    break;

  case AMQP_RESPONSE_LIBRARY_EXCEPTION:
    if (x.library_error != -13) //超时状态不记日志 ，日志太多
      LOG_ERROR << context <<" : " << amqp_error_string2(x.library_error);
    break;

  case AMQP_RESPONSE_SERVER_EXCEPTION:
    switch (x.reply.id) {
    case AMQP_CONNECTION_CLOSE_METHOD: {
      amqp_connection_close_t *m = (amqp_connection_close_t *) x.reply.decoded;
      LOG_ERROR << context  << " : server connection error " << m->reply_code << " , message: " << (char *)m->reply_text.bytes;
      break;
    }
    case AMQP_CHANNEL_CLOSE_METHOD: {
      amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
      LOG_ERROR << context  << " : server channel error " << m->reply_code << " , message: " << (char *)m->reply_text.bytes;
      break;
    }
    default:
      LOG_ERROR << context << " : unknown server error, method id " << x.reply.id;
      fprintf(stderr, "%s: unknown server error, method id 0x%08X\n", context, x.reply.id);
      break;
    }
    break;
  }

  return true;
}

static void dump_row(long count, int numinrow, int *chs)
{
  int i;

  printf("%08lX:", count - numinrow);

  if (numinrow > 0) {
    for (i = 0; i < numinrow; i++) {
      if (i == 8) {
        printf(" :");
      }
      printf(" %02X", chs[i]);
    }
    for (i = numinrow; i < 16; i++) {
      if (i == 8) {
        printf(" :");
      }
      printf("   ");
    }
    printf("  ");
    for (i = 0; i < numinrow; i++) {
      if (isprint(chs[i])) {
        printf("%c", chs[i]);
      } else {
        printf(".");
      }
    }
  }
  printf("\n");
}

static int rows_eq(int *a, int *b)
{
  int i;

  for (i=0; i<16; i++)
    if (a[i] != b[i]) {
      return 0;
    }

  return 1;
}

void amqp_dump(void const *buffer, size_t len)
{
  unsigned char *buf = (unsigned char *) buffer;
  long count = 0;
  int numinrow = 0;
  int chs[16];
  int oldchs[16] = {0};
  int showed_dots = 0;
  size_t i;

  for (i = 0; i < len; i++) {
    int ch = buf[i];

    if (numinrow == 16) {
      int j;

      if (rows_eq(oldchs, chs)) {
        if (!showed_dots) {
          showed_dots = 1;
          printf("          .. .. .. .. .. .. .. .. : .. .. .. .. .. .. .. ..\n");
        }
      } else {
        showed_dots = 0;
        dump_row(count, numinrow, chs);
      }

      for (j=0; j<16; j++) {
        oldchs[j] = chs[j];
      }

      numinrow = 0;
    }

    count++;
    chs[numinrow++] = ch;
  }

  dump_row(count, numinrow, chs);

  if (numinrow != 0) {
    printf("%08lX:\n", count);
  }
}

int init_rabbitmq(amqp_connection_state_t &conn,std::string hostname, int port, std::string user, std::string passwd)
{
  conn = amqp_new_connection();
  amqp_socket_t *socket = amqp_tcp_socket_new(conn);
  if (!socket) {
    fprintf(stderr,"amqp_tcp_socket_new error");
    LOG_ERROR << "RabbitMqConsumer amqp_tcp_socket_new error";
    return -1;
  }

  int status = amqp_socket_open(socket, hostname.c_str(), port);
  if (status) {
    fprintf(stderr,"amqp_socket_open error [%s] [%d]", hostname.c_str(), port);
    LOG_ERROR << "RabbitMqConsumer opening TCP socket error";
    return -1;
  }

  int res = die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, user.c_str(), passwd.c_str()),
                    "Logging in");
  if (res)
  {
    fprintf(stderr,"amqp_login error [%s]", user.c_str());
    return -1;
  }
  amqp_channel_open(conn, 1);
  res = die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");
  if (res)
  {
    fprintf(stderr,"amqp_channel_open error ");
    return -1;
  }
  return 0;
}

int queue_declare(amqp_connection_state_t &conn, std::string queuename, bool purge, bool ttl)
{
	int res = 0;
	if (purge)
	{
		amqp_queue_purge(conn, 1, amqp_cstring_bytes(queuename.c_str()));
		res = die_on_amqp_error(amqp_get_rpc_reply(conn), "Clearing spot queue");
		if (res)
		{
			LOG_WARN << "purge failed. queue maybe not exist. queuename:" << queuename << " res:" << res;
		}
	}
	if (ttl)
	{
		//set ttl for spotmd
		amqp_bytes_t bytes = amqp_cstring_bytes("x-message-ttl");
		amqp_table_entry_t entry;
		entry.key = bytes;
		entry.value.kind = AMQP_FIELD_KIND_I32;
		entry.value.value.i32 = 10000;
		amqp_table_t att;
		att.num_entries = 1;
		att.entries = &entry;
		LOG_INFO << "amqp_queue_declare ttl:10000,queue name:" << queuename;
		amqp_queue_declare(conn, 1, amqp_cstring_bytes(queuename.c_str()), 0, 1, 0, 0, att);
	}
	else
	{
		LOG_INFO << "amqp_queue_declare ttl:empty,queue name:" << queuename;
		amqp_queue_declare(conn, 1, amqp_cstring_bytes(queuename.c_str()), 0, 1, 0, 0, amqp_empty_table);
	}
	res = die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring spot queue");
	if (res)
	{
		fprintf(stderr, "amqp_queue_declare error [%s]", queuename.c_str());
		return -1;
	}

	return 0;
}

int queue_purge(amqp_connection_state_t &conn, std::string queuename)
{
	int res = 0;
	amqp_queue_purge(conn, 1, amqp_cstring_bytes(queuename.c_str()));
	res = die_on_amqp_error(amqp_get_rpc_reply(conn), "Clearing spot queue");
	if (res)
	{
		LOG_FATAL << "purge failed. queue maybe not exist. queuename:" << queuename << " res:" << res;
	}

	return res;
}