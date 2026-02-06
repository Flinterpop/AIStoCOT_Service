// AISTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "mongoose.h"


#include "BG_SocketBase.h"
#include "COTSender.h"
#include "bg_TakMessage.h"
#include "AIStoCoT.h"



bool g_debug = true;



// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved
//
// Example Websocket server with timers. This is a simple Websocket echo
// server, which sends a message to all connected clients periodically,
// using timer API.

#include "mongoose.h"

static const char* s_listen_on = "http://localhost:8800";
static const char* s_web_root = "web_root";





////NTP section


// The UNIX epoch of the boot time. Initially, we set it to 0.  But then after
// SNTP response, we update it to the correct value, which will allow us to
// use time(). Uptime in milliseconds is returned by mg_millis().
static time_t s_boot_timestamp = 0;

// SNTP client connection
static struct mg_connection* s_sntp_conn = NULL;

// On embedded systems, rename to time()
time_t my_time(time_t* tp) {
	// you can just return mg_now() / 1000;
	time_t t = s_boot_timestamp + mg_millis() / 1000;
	if (tp != NULL) *tp = t;
	return t;
}

// SNTP client callback
static void sfn(struct mg_connection* c, int ev, void* ev_data) {
	if (ev == MG_EV_SNTP_TIME) {
		// Time received, the internal protocol handler updates what mg_now() returns
		uint64_t curtime = mg_now();
		MG_INFO(("SNTP-updated current time is: %llu ms from epoch", curtime));
		printf("SNTP-updated current time is: %llu ms from epoch", curtime);
		// otherwise, you can process the server returned data yourself
		{
			uint64_t t = *(uint64_t*)ev_data;
			s_boot_timestamp = (time_t)((t - mg_millis()) / 1000);
			MG_INFO(("Got SNTP time: %llu ms from epoch, ", t));
			printf("Got SNTP time: %llu ms from epoch, ", t);
		}
	}
	else if (ev == MG_EV_CLOSE) {
		s_sntp_conn = NULL;
	}
	(void)c;
}


// Called every 5 seconds. Increase that for production case.
static void timer_fn2(void* arg) {
	struct mg_mgr* mgr = (struct mg_mgr*)arg;
	if (s_sntp_conn == NULL) { // connection issues a request
		s_sntp_conn = mg_sntp_connect(mgr, NULL, sfn, NULL);
	}
	else {
		mg_sntp_request(s_sntp_conn);
	}
}





//Websocket section

// This RESTful server implements the following endpoints:
static void fn(struct mg_connection* c, int ev, void* ev_data) {
	if (ev == MG_EV_HTTP_MSG) {
		struct mg_http_message* hm = (struct mg_http_message*)ev_data;
		if (mg_match(hm->uri, mg_str("/websocket"), NULL)) {
			mg_ws_upgrade(c, hm, NULL);  // Upgrade HTTP to Websocket
			c->data[0] = 'W';           // Set some unique mark on a connection
		}
		else {
			// Serve static files
			struct mg_http_serve_opts opts = { .root_dir = s_web_root };
			mg_http_serve_dir(c, (mg_http_message *)ev_data, &opts);
		}
	}
	else if (ev == MG_EV_WS_MSG) {
		// Got websocket frame. Received data is wm->data. Echo it back!
		struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
		mg_ws_send(c, wm->data.buf, wm->data.len, WEBSOCKET_OP_TEXT);
		mg_iobuf_del(&c->recv, 0, c->recv.len);
	}
}

static void timer_fn(void* arg) {
	struct mg_mgr* mgr = (struct mg_mgr*)arg;
	// Broadcast "hi" message to all connected websocket clients.
	// Traverse over all connections
	for (struct mg_connection* c = mgr->conns; c != NULL; c = c->next) {
		// Send only to marked connections
		if (c->data[0] == 'W') mg_ws_send(c, "hi", 2, WEBSOCKET_OP_TEXT);
	}
}


//////////




int val = 1;
static void ev_handler(struct mg_connection* c, int ev, void* ev_data) {
	if (ev == MG_EV_HTTP_MSG) {
		struct mg_http_message* hm = (struct mg_http_message*)ev_data;
		if (mg_match(hm->uri, mg_str("/api/led/get"), NULL)) {
			mg_http_reply(c, 200, "", "%d\n", val++);
		}
		else if (mg_match(hm->uri, mg_str("/api/led/toggle"), NULL))
		{
			//HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0); // Can be different on your board
			mg_http_reply(c, 200, "", "true\n");
		}
		else {
			struct mg_http_message* hm = (struct mg_http_message*)ev_data;
			struct mg_http_serve_opts opts = { .root_dir = s_web_root };
			mg_http_serve_dir(c, hm, &opts);
		}
	}
}


/////////////

int main(void) {
	struct mg_mgr mgr;        // Event manager
	mg_mgr_init(&mgr);        // Initialise event manager
	mg_log_set(MG_LL_DEBUG);  // Set log level
	
	mg_timer_add(&mgr, 5000, MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, timer_fn2, &mgr);

	mg_timer_add(&mgr, 1000, MG_TIMER_REPEAT, timer_fn, &mgr);


	mg_http_listen(&mgr, s_listen_on, fn, NULL);  // Create HTTP listener
	for (;;) mg_mgr_poll(&mgr, 500);              // Infinite event loop
	mg_mgr_free(&mgr);                            // Free manager resources
	return 0;
}


/*

static const char* s_listen_on = "http://localhost:8800";
static const char* s_web_root = "web_root";






int val = 1;
static void ev_handler(struct mg_connection* c, int ev, void* ev_data) {
	if (ev == MG_EV_HTTP_MSG) {
		struct mg_http_message* hm = (struct mg_http_message*)ev_data;
		if (mg_match(hm->uri, mg_str("/api/led/get"), NULL)) {
			mg_http_reply(c, 200, "", "%d\n", val++);
		}
		else if (mg_match(hm->uri, mg_str("/api/led/toggle"), NULL)) 
		{
			//HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0); // Can be different on your board
			mg_http_reply(c, 200, "", "true\n");
		}
		else {
			struct mg_http_message* hm = (struct mg_http_message*)ev_data;
			struct mg_http_serve_opts opts = { .root_dir = s_web_root };
			mg_http_serve_dir(c, hm, &opts);
		}
	} 
}

static void timer_fn(void* arg) {
	struct mg_mgr* mgr = (struct mg_mgr*)arg;
	// Broadcast "hi" message to all connected websocket clients.
	// Traverse over all connections
	for (struct mg_connection* c = mgr->conns; c != NULL; c = c->next) 
	{
		puts("Here");
		// Send only to marked connections
		if (c->data[0] == 'W') mg_ws_send(c, "hi", 2, WEBSOCKET_OP_TEXT);
	}
}


int main()
{

	initialise_winsock();
	getNetworkAdapterInfo();
	std::string retVal = COTSENDER::StartCOTSender();

	std::vector<std::string> nmeaList;

	nmeaList.push_back("!AIVDM,1,1,,A,1Cu?etPjh0KT>H@I;dL1hVv00000,0*57");
	nmeaList.push_back("!AIVDM,2,1,0,A,5Cu?etP00000<L4`000<P58hEA@E@uLp0000000S>8OA;0jjf012AhV@,0*47");
	nmeaList.push_back("!AIVDM,2,2,0,A,000000000000000,2*24");


	for (std::string nmea : nmeaList)
		AIS2COT::ProcessNMEAToCoT(nmea);


	struct mg_mgr mgr;  // Declare event manager
	mg_mgr_init(&mgr);  // Initialise event manager
	
	mg_timer_add(&mgr, 1000, MG_TIMER_REPEAT, timer_fn, &mgr);

	mg_http_listen(&mgr, s_listen_on, ev_handler, NULL);  // Setup listener
	for (;;) {          // Run an infinite event loop
		mg_mgr_poll(&mgr, 1000);
	}

	COTSENDER::StopCOTSender();
	closeandclean_winsock();

}

*/