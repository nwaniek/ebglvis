/*************************************************************************
 * aertcpsample.c - version 150501
 *
 * Real-time sampling of AER events over TCP using Matlab/Mex.
 * Written by Fredrik Sandin at CapoCaccia 2015, fredrik.sandin@gmail.com
 *************************************************************************
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *************************************************************************
 * Usage in Matlab:
 * ----------------
 * >> compile
 * >> aertcpsample('start');
 * >> [x,y,polarity,timestamp] = aertcpsample('receive');
 * >> aertcpsample('stop');
 * >> % or run the example
 *
 ************************************************************************/

#if 0

#include <mex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PRINTF_LOG 1
#include "nets.h"
#include "common.h"
#include "polarity.h"

/*******************************************************************/

static bool gStarted = false, gStop = false;

struct pevent_t {
    uint16_t x;
    uint16_t y;
    bool     polarity;
    uint32_t timestamp;
};

const char *ipAddress         = "10.10.0.130"; // IP
uint16_t portNumber           = 7777;          // port
int listenTCPSocket           = 0;

const size_t dataBufferLength = 1024 * 64;     // 64K buffer
uint8_t *dataBuffer           = 0;

const size_t eventBufferLength = 1024 * 128;   // 128K events
struct pevent_t *eventBuffer   = 0;            // circular event buffer
size_t eventWriteIndex         = 0;
size_t eventReadIndex          = 0;

static pthread_t gSamplingThread;

// Mutex used to avoid simultaneous access to dataBuffer and eventBuffer
static pthread_mutex_t gDataMutex = PTHREAD_MUTEX_INITIALIZER;

/*******************************************************************/

void *sampling_thread(void *threadId) {

	while (!gStop) {

        pthread_mutex_lock( &gDataMutex );

		// Get packet header
		if (!recvUntilDone(listenTCPSocket, dataBuffer, sizeof(struct caer_event_packet_header))) {
			mexErrMsgTxt("Error in recv() call"); // : %d\n", errno);
		}

		// Decode header
		caerEventPacketHeader header = (caerEventPacketHeader)dataBuffer;
		uint16_t eventType = caerEventPacketHeaderGetEventType(header);
		uint16_t eventSource = caerEventPacketHeaderGetEventSource(header);
		uint32_t eventSize = caerEventPacketHeaderGetEventSize(header);
		uint32_t eventTSOffset = caerEventPacketHeaderGetEventTSOffset(header);
		uint32_t eventCapacity = caerEventPacketHeaderGetEventCapacity(header);
		uint32_t eventNumber = caerEventPacketHeaderGetEventNumber(header);
		uint32_t eventValid = caerEventPacketHeaderGetEventValid(header);

        if(eventType == POLARITY_EVENT) {

            caerPolarityEventPacket polpack = (caerPolarityEventPacket)dataBuffer;

            // Get rest of event packet, the part with the events themselves
            if (!recvUntilDone(listenTCPSocket, dataBuffer + sizeof(struct caer_event_packet_header),
                           eventCapacity * eventSize)) {
                mexErrMsgTxt("Error in recv() call"); // : %d\n", errno);
            }

            // For each event
            for(unsigned i=0; i<eventValid; i++) {

                caerPolarityEvent event = caerPolarityEventPacketGetEvent(polpack, i);

                eventBuffer[eventWriteIndex].x = caerPolarityEventGetX(event);
                eventBuffer[eventWriteIndex].y = caerPolarityEventGetY(event);
                eventBuffer[eventWriteIndex].polarity = caerPolarityEventGetPolarity(event);
                eventBuffer[eventWriteIndex].timestamp = caerPolarityEventGetTimestamp(event);

                if(++eventWriteIndex >= eventBufferLength) eventWriteIndex=0; // Wrap around at end of buffer

                /*
                printf("polarity event (%d,%d) at time %d\n",eventBuffer[eventWriteIndex].x,
                                                             eventBuffer[eventWriteIndex].y,
                                                             eventBuffer[eventWriteIndex].timestamp);
                */
            }
        }

        pthread_mutex_unlock( &gDataMutex );
    }
	pthread_exit(NULL);
}


/*******************************************************************/

void cmd_start(void) {

    if(gStarted) mexErrMsgTxt("AER sampling already started");

	dataBuffer = mxMalloc(dataBufferLength);
    mexMakeMemoryPersistent(dataBuffer);

    eventBuffer = mxMalloc(eventBufferLength * sizeof(struct pevent_t));
    mexMakeMemoryPersistent(eventBuffer);

	listenTCPSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenTCPSocket < 0) {
        mxFree(dataBuffer);
        mxFree(eventBuffer);
		mexErrMsgTxt("Failed to create TCP socket");
	}

	struct sockaddr_in listenTCPAddress;
	memset(&listenTCPAddress, 0, sizeof(struct sockaddr_in));

	listenTCPAddress.sin_family = AF_INET;
	listenTCPAddress.sin_port = htons(portNumber);
	inet_aton(ipAddress, &listenTCPAddress.sin_addr); // htonl() is implicit here.

	if(connect(listenTCPSocket, (struct sockaddr *) &listenTCPAddress, sizeof(struct sockaddr_in)) < 0) {
        mxFree(dataBuffer);
        mxFree(eventBuffer);
		mexErrMsgTxt("Failed to connect to remote TCP data server");
	}

    if(pthread_create(&gSamplingThread, NULL, sampling_thread, NULL)!=0) {
        mxFree(dataBuffer);
        mxFree(eventBuffer);
        mexErrMsgTxt("Error in pthread_create()");
    }
    gStarted = true;
    gStop = false;
}

void cmd_stop(void) {
    if(!gStarted) {
        mexErrMsgTxt("AER sampling not started");
    } else {
        gStop = true;
        pthread_join(gSamplingThread, NULL);
        close(listenTCPSocket);
        mxFree(dataBuffer);
        mxFree(eventBuffer);
        gStarted = false;
        gStop = false;
    }
}

void at_exit(void) {
	if(gStarted) {
		gStop = true;
        close(listenTCPSocket);
		pthread_join(gSamplingThread, NULL);
        mxFree(dataBuffer);
        mxFree(eventBuffer);
	}
}

/*******************************************************************/

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    // Set exit callback
	mexAtExit(&at_exit);

	// Check input arguments
	char strbuf[256];
	if(nrhs<1)
		mexErrMsgTxt("Empty argument list not expected");
	if(mxGetString(prhs[0], strbuf, sizeof(strbuf)/sizeof(char))!=0)
		mexErrMsgTxt("Expected first argument to be a string (command)");

	// Execute requested task
	if(strcmp("start", strbuf)==0) {
        cmd_start();
	} else if(strcmp("stop", strbuf)==0) {
        cmd_stop();
	} else if(strcmp("receive", strbuf)==0) {

		if(!gStarted)
			mexErrMsgTxt("Sampling not started, empty buffer");
		if(nlhs!=4)
			mexErrMsgTxt("Expected four output variables, [x,y,polarity,timestamp]");

        pthread_mutex_lock( &gDataMutex );

        // Calculate number of events in buffer
		size_t numevnts = 0, i=0;
        if(eventReadIndex <= eventWriteIndex) {
            numevnts = eventWriteIndex - eventReadIndex;
        } else {
            numevnts = eventBufferLength - eventReadIndex; // Read to end
            numevnts += eventWriteIndex; // Read remaining events
        }

        //  Create output vectors
        mwSize dims[] = {numevnts};
		plhs[0] = mxCreateNumericArray(1, dims, mxUINT16_CLASS, mxREAL); // x
		plhs[1] = mxCreateNumericArray(1, dims, mxUINT16_CLASS, mxREAL); // y
		plhs[2] = mxCreateNumericArray(1, dims, mxUINT8_CLASS, mxREAL);  // polarity
		plhs[3] = mxCreateNumericArray(1, dims, mxUINT32_CLASS, mxREAL); // timestamp
        uint16_t *x = (uint16_t*)mxGetPr(plhs[0]);
        uint16_t *y = (uint16_t*)mxGetPr(plhs[1]);
        uint8_t  *p = (uint8_t*)mxGetPr(plhs[2]);
        uint32_t *t = (uint32_t*)mxGetPr(plhs[3]);

        // Copy events from circular buffer
        while(numevnts-- > 0) {
            x[i] = eventBuffer[eventReadIndex].x;
            y[i] = eventBuffer[eventReadIndex].y;
            p[i] = eventBuffer[eventReadIndex].polarity;
            t[i] = eventBuffer[eventReadIndex].timestamp;
            i++;
            if(++eventReadIndex >= eventBufferLength)
                eventReadIndex=0; // Wrap around at end of buffer
        }

		pthread_mutex_unlock( &gDataMutex );
	}
}

/
#endif
