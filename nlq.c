/*
 *   LIBNLQ: Netlink Queue library
 *   Copyright (C) 2018  Renzo Davoli <renzo@cs.unibo.it>
 *   VirtualSquare team.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libnlq.h>

struct nlq_msg *nlq_createmsg(uint16_t nlmsg_type, uint16_t nlmsg_flags, uint32_t nlmsg_seq, uint32_t nlmsg_pid) {
	struct nlq_msg *nlq_msg = malloc(sizeof(struct nlq_msg));
	if (nlq_msg != NULL) {
		nlq_msg->nlq_packet = NULL;
		nlq_msg->nlq_size = 0;
		nlq_msg->nlq_file = open_memstream((char **) &nlq_msg->nlq_packet, &nlq_msg->nlq_size);
		if (nlq_msg->nlq_file == NULL) {
			free(nlq_msg);
			return NULL;
		} else {
			static _Atomic int seq;
			struct nlmsghdr hdr = {
				.nlmsg_len = 0,
				.nlmsg_type = nlmsg_type,
				.nlmsg_flags = nlmsg_flags,
				.nlmsg_seq = (nlmsg_flags & NLM_F_REQUEST) && nlmsg_seq == 0 ? ++seq : nlmsg_seq,
				.nlmsg_pid = nlmsg_pid
			};
			nlq_add(nlq_msg, &hdr, sizeof(hdr));
			return nlq_msg;
		}
	} else
		return NULL;
}

void nlq_complete(struct nlq_msg *nlq_msg) {
	fclose(nlq_msg->nlq_file);
	if (nlq_msg->nlq_size >= sizeof(struct nlmsghdr))
		nlq_msg->nlq_packet->nlmsg_len = nlq_msg->nlq_size;
	nlq_msg->nlq_next = NULL;
}

void nlq_freemsg(struct nlq_msg *nlq_msg) {
	free(nlq_msg->nlq_packet);
	free(nlq_msg);
}

void nlq_enqueue(struct nlq_msg *nlq_msg, struct nlq_msg **nlq_tail) {
	if (nlq_msg->nlq_next == NULL)
		nlq_msg->nlq_next = nlq_msg;
	if ((*nlq_tail) != NULL) {
		struct nlq_msg *nlq_first = (*nlq_tail)->nlq_next;
		(*nlq_tail)->nlq_next = nlq_msg->nlq_next;
		nlq_msg->nlq_next = nlq_first;
	}
	*nlq_tail = nlq_msg;
}

struct nlq_msg *nlq_head(struct nlq_msg *nlq_tail) {
  if (nlq_tail == NULL)
    return NULL;
  else
    return nlq_tail->nlq_next;
}

struct nlq_msg *nlq_dequeue(struct nlq_msg **nlq_tail) {
	struct nlq_msg *first = nlq_head(*nlq_tail);
	if (first != NULL) {
		if (first->nlq_next == first)
			*nlq_tail = NULL;
		else {
			(*nlq_tail)->nlq_next = first->nlq_next;
			first->nlq_next = first;
		}
	}
	return first;
}

int nlq_length(struct nlq_msg *nlq_tail) {
	if (nlq_tail == NULL)
		return 0;
	else {
		struct nlq_msg *nlq_scan;
		int count;
		for (nlq_scan = nlq_tail->nlq_next, count = 1;
				nlq_scan != nlq_tail;
				nlq_scan = nlq_scan->nlq_next)
			count++;
		return count;
	}
}

void nlq_free (struct nlq_msg **nlq_tail) {
	while (*nlq_tail != NULL)
		nlq_freemsg(nlq_dequeue(nlq_tail));
}

void nlq_addattr(struct nlq_msg *nlq_msg, unsigned short nla_type, const void *nla_data, unsigned short nla_datalen) {
	struct nlattr nla = {
		.nla_len = sizeof(struct nlattr) + nla_datalen,
		.nla_type = nla_type
	};
	nlq_add(nlq_msg, &nla, sizeof(nla));
	nlq_add(nlq_msg, nla_data, nla_datalen);
}

struct nlq_msg *nlq_createxattr(void) {
  struct nlq_msg *nlq_msg = malloc(sizeof(struct nlq_msg));
  if (nlq_msg != NULL) {
    nlq_msg->nlq_packet = NULL;
    nlq_msg->nlq_size = 0;
    nlq_msg->nlq_file = open_memstream((char **) &nlq_msg->nlq_packet, &nlq_msg->nlq_size);
    if (nlq_msg->nlq_file == NULL) {
      free(nlq_msg);
      return NULL;
    } else
      return nlq_msg;
  } else
    return NULL;
}

void nlq_addxattr(struct nlq_msg *nlq_msg, unsigned short nla_type, struct nlq_msg *xattr) {
	struct nlattr nla = {
    .nla_len = sizeof(struct nlattr),
    .nla_type = nla_type
  };
	fclose(xattr->nlq_file);
	nla.nla_len += xattr->nlq_size;
	nlq_add(nlq_msg, &nla, sizeof(nla));
	nlq_add(nlq_msg, xattr->nlq_packet, xattr->nlq_size);
	nlq_freemsg(xattr);
}
