// AISTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "mongoose.h"


#include "BG_SocketBase.h"
#include "COTSender.h"
#include "bg_TakMessage.h"
#include "AIStoCoT.h"



bool g_debug = true;


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
			struct mg_http_serve_opts opts = { .root_dir = "./web_root/" };
			mg_http_serve_dir(c, hm, &opts);
		}
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
	mg_http_listen(&mgr, "http://0.0.0.0:8800", ev_handler, NULL);  // Setup listener
	for (;;) {          // Run an infinite event loop
		mg_mgr_poll(&mgr, 1000);
	}

	COTSENDER::StopCOTSender();
	closeandclean_winsock();

}

