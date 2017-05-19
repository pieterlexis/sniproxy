/*
 * Copyright (c) 2017, Pieter Lexis <pieter.lexis@powerdns.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "address.h"
#include "logger.h"
#include "stats.h"

struct Statistics * init_statistics() {
  struct Statistics *statistics = malloc(sizeof(struct Statistics));
  statistics->numIncomingConns = 0;
  statistics->numOutGoingConns = 0;
  statistics->numErroredOutgoingConns = 0;
  statistics->numBytesIn = 0;
  statistics->numBytesOut = 0;

  return statistics;
}

int
sendStatToWire(const char * msg, int sockfd) {
  ssize_t sent = send(sockfd, msg, strlen(msg), MSG_MORE);
  if (sent < 0) {
    err("Unable to send data: %s", strerror(errno));
  }
  else if (sent != strlen(msg)) {
    err("Not all data was sent!");
  }
  return sent;
}

void cleanupAll(int sockfd, char * msg) {
  if (close(sockfd) < 0) {
    err("Unable to close socket: %s", strerror(errno));
  }
  free(msg);
}

void
sendStats(const struct Statistics * statistics, const struct Address * address, const char* myname) {

  unsigned long now = time(NULL);
  char * msg = NULL;

  const struct sockaddr *sa_address = address_sa(address);
  socklen_t sa_address_len = address_sa_len(address);

  int sockfd = socket(sa_address->sa_family, SOCK_STREAM, 0);
  if (sockfd < 0) {
    err("socket(): %s", __func__);
    return;
  }

  for (int attempt = 0; attempt <= 2; attempt++){
    if (connect(sockfd, sa_address, sa_address_len) == -1) {
      char addr[128+1+5+1]; // 128 chars IPv6, ':', port, '\0'
      err("Unable to connect to remote carbon host %s: %s",
          display_sockaddr(sa_address, addr, sizeof(addr)),
          strerror(errno));
      if(attempt == 2) {
        cleanupAll(sockfd, msg);
        return;
      }
      sleep(3);
      continue;
    }
    break;
  }

  if(asprintf(&msg, "sniproxy.%s.num-incoming-conns %i %lu\n",
      myname, statistics->numIncomingConns,
      now) < 0){
    err("%s: asprintf", __func__);
    cleanupAll(sockfd, msg);
    return;
  }

  if (sendStatToWire(msg, sockfd) < 0 ) {
    cleanupAll(sockfd, msg);
    return;
  }

  if(asprintf(&msg, "sniproxy.%s.num-outgoing-conns %i %lu\n",
      myname, statistics->numOutGoingConns,
      now) < 0){
    err("%s: asprintf", __func__);
    cleanupAll(sockfd, msg);
    return;
  }

  if (sendStatToWire(msg, sockfd) < 0 ) {
    cleanupAll(sockfd, msg);
    return;
  }

  if(asprintf(&msg, "sniproxy.%s.num-error-outgoing-conns %i %lu\n",
      myname, statistics->numErroredOutgoingConns,
      now) < 0){
    err("%s: asprintf", __func__);
    cleanupAll(sockfd, msg);
    return;
  }

  if (sendStatToWire(msg, sockfd) < 0 ) {
    cleanupAll(sockfd, msg);
    return;
  }

  if(asprintf(&msg, "sniproxy.%s.num-bytes-in %i %lu\n",
      myname, statistics->numBytesIn,
      now) < 0){
    err("%s: asprintf", __func__);
    cleanupAll(sockfd, msg);
    return;
  }

  if (sendStatToWire(msg, sockfd) < 0 ) {
    cleanupAll(sockfd, msg);
    return;
  }

  if(asprintf(&msg, "sniproxy.%s.num-bytes-out %i %lu\n",
      myname, statistics->numBytesOut,
      now) < 0){
    err("%s: asprintf", __func__);
    cleanupAll(sockfd, msg);
    return;
  }

  if (sendStatToWire(msg, sockfd) < 0 ) {
    cleanupAll(sockfd, msg);
    return;
  }

  // Flushes the data
  cleanupAll(sockfd, msg);
}
