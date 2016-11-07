/*
 * Copyright (c) 2010-2011 Csaba Kiraly
 *
 * This file is part of PeerStreamer.
 *
 * PeerStreamer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PeerStreamer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with PeerStreamer.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <stdint.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>

#include "transaction.h"
#include "measures.h"
#include "dbg.h"

#define MAX(A,B)    ((A)>(B) ? (A) : (B))
#define MIN(A,B)    ((A)<(B) ? (A) : (B))

bool autotune_period = true;

static double offer_accept = 1;
static double acc_to_ack = 0;

double offer_accept_smoothing = 0.9;	// %
double offer_accept_min = 0.3;	// %
double acc_to_ack_max = 0.300;	// sec
double period_change_rate_up = 0.2;	// %/sec
double period_change_rate_down = 0.1;	// %/sec

extern struct timeval period;
#define PERIOD_MIN 5000
#define PERIOD_MAX 1000000

int64_t tv2int(struct timeval *tv)
{
  return tv->tv_sec * 1000000 + tv->tv_usec;
}

void int2tv(struct timeval *tv, int64_t t)
{
  tv->tv_sec = t / 1000000;
  tv->tv_usec = t % 1000000;
}

static int64_t get_period()
{
  return tv2int(&period);
}

static void set_period(int64_t p)
{
  reg_period(p);
  int2tv(&period, p);
}

static void update_period()
{
  static struct timeval period_last_updated;
  struct timeval t_now;
  int64_t dt;
  double d_offer_accept;
  double d_acc_to_ack;
  static int64_t period_initial;
  double d_period;

  if (! period_initial) period_initial = get_period();

  gettimeofday(&t_now, NULL);

  dprintf("update_period: offer_accept=%f acc_to_ack=%f period=%lu\n", offer_accept, acc_to_ack, get_period());

  if (timerisset(&period_last_updated)) {
    dt = MIN(tv2int(&t_now) - tv2int(&period_last_updated), PERIOD_MAX);
  } else {
    dt = tv2int(&period);
  }
  period_last_updated = t_now;

  d_offer_accept = offer_accept - offer_accept_min;
  d_acc_to_ack = (acc_to_ack - acc_to_ack_max) / acc_to_ack_max;
  d_period = log(get_period()) - log(period_initial);

  if (d_period < 0) { // we are fast
    if (d_offer_accept > 0 && d_acc_to_ack < 0) { // be faster
      set_period(MAX(PERIOD_MIN, get_period() / (1.0 + (period_change_rate_down * dt / 1e6))));
    } else { //slow down towards normal speed
      set_period(MIN(PERIOD_MAX, get_period() * (1.0 + (period_change_rate_up * (1 + fabs(d_period < 0 ? d_period : 0) ) * dt / 1e6))));
    }
  } else { // we are slow
    if (d_acc_to_ack > 0) { // slow down even more
      set_period(MIN(PERIOD_MAX, get_period() * (1.0 + (period_change_rate_up * dt / 1e6))));
    } else { //recover speed
      set_period(MAX(PERIOD_MIN, get_period() / (1.0 + (period_change_rate_down * (1 + fabs(d_period > 0 ? d_period : 0) ) * dt / 1e6))));
    }
  }
}

static void update_offer_accept(bool accepted)
{
  offer_accept = offer_accept * offer_accept_smoothing + (accepted ? 1 - offer_accept_smoothing : 0);
}

static void update_acc_to_ack(double t)
{
  reg_queue_delay(t);
  acc_to_ack = t;
  if (autotune_period) {
    update_period();
  }
}


void rc_reg_accept(uint16_t trans_id, int accepted)
{
  update_offer_accept(accepted);
}

void rc_reg_ack(uint16_t trans_id)
{
  double t_acc, t_acc_to_ack;
  struct timeval t_now;

  t_acc = transaction_remove(trans_id);

  if (t_acc < 0) {
    dprintf(" can't find transaction for trans_id %d.\n", trans_id);
    return;
  }

  gettimeofday(&t_now, NULL);
  t_acc_to_ack = t_now.tv_sec + t_now.tv_usec*1e-6 - t_acc;

  update_acc_to_ack(t_acc_to_ack);
}