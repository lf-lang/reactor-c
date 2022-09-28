/**
 * @file
 * @author Edward A. Lee
 * @author Erling Jellum
 * @author Christian Menard
 * @author Marten Lohstroh
 *
 * @section LICENSE
Copyright (c) 2022, The University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * @section DESCRIPTION
 * Header file for the runtime infrastructure for distributed Lingua Franca programs.
 *
*/

#ifndef AFFILIATE_H
#define AFFILIATE_H

#include "reactor.h"

/////////////////////////////////////////////
//// Data structures

/** State of an affiliate during execution. */
typedef enum {
    NOT_CONNECTED,  // The affiliate has not connected.
    GRANTED,        // Executing.
    PENDING         // Waiting for upstream affiliates.
} affiliate_state_t;

/**
 * Information about an affiliate.
 * The list of upstream and downstream affiliates does not include
 * those that are connected via a "physical" connection (one
 * denoted with ~>) because those connections do not impose
 * any scheduling constraints.
 */
typedef struct {
    uint16_t id;            // ID of this affiliate.
    tag_t completed;        // The largest logical tag completed by the affiliate (or NEVER if no LTC has been received).
    tag_t last_granted;     // The maximum TAG that has been granted so far (or NEVER if none granted)
    tag_t last_provisionally_granted;      // The maximum PTAG that has been provisionally granted (or NEVER if none granted)
    tag_t next_event;       // Most recent NET received from the federate (or NEVER if none received).
    fed_state_t state;      // State of the federate.
    int* upstream;          // Array of upstream federate ids.
    interval_t* upstream_delay;    // Minimum delay on connections from upstream federates.
    							   // Here, NEVER encodes no delay. 0LL is a microstep delay.
    int num_upstream;              // Size of the array of upstream federates and delays.
    int* downstream;        // Array of downstream federate ids.
    int num_downstream;     // Size of the array of downstream federates.
    bool requested_stop;    // Indicates that the federate has requested stop or has replied
                            // to a request for stop from the RTI. Used to prevent double-counting
                            // a federate when handling lf_request_stop().
} affiliate_state_t;

typedef struct {
    pqueue_t* event_queue;
    affiliate_state_t state;
} affiliate_t;

#endif // AFFILIATE_H