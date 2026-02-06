#pragma once

#include "ImGui.h"
#include <vector>
#include <utility>  // For std::pair

#include "ImBGUtil.h"

#include <inttypes.h> 

#include "FeatureModuleBase.h"


class COTModule :FeatureModuleBase
{
public:
    COTModule() : FeatureModuleBase() 
    {
        moduleName = "COT";
    };

public:
    void Initialize(ImVec2(*funcLatLon2VPxy)(double, double))  override;
    void ShutDown()  override;

    void LoadIniValues()   override;
    void SaveIniValues()   override;

    void Menu()  override;

    void ShowDialogs() override;


private:

    void ShowCOTControlDialog(bool* pOpen);
    
    bool mb_ShowEntityList = true;
    bool mb_ShowCOTMessageLog = true;
    bool mb_ShowCOTMsgBuilder = false;
    bool mb_ShowCOTChatLog = false;


};



struct st_XMLDetail
{
	std::string x_callSign{};
	std::string x_uid{};
	std::string x_remarks{};
	int x_URN{};


	//elements of a standard ATAK 5.4 text
	std::string parent{};
	std::string groupOwner{};
	std::string messageId{};
	std::string chatRoom{};
	std::string id{};
	std::string senderCallSign{};
	std::string chatgrp_uid0{};
	std::string chatgrp_uid1{};
	std::string chatgrp_id{};

	std::string link_uid{};
	std::string link_type{};
	std::string link_relation{};

	std::string __serverdestination_destinations{};

	std::string remarks_source{};
	std::string remarks_to{};
	std::string remarks_time{};
	std::string remarks_text{};

};


struct Contact {
	// Endpoint is optional; if missing/empty do not populate.
	std::string endpoint{};// = 1;           // endpoint=
	std::string callsign{};// = 2;           // callsign=
};

struct Group {
	std::string name{};// = 1;           // name=
	std::string role{};// = 2;           // role=
};

struct PrecisionLocation {
	std::string geopointsrc{};// = 1;        // geopointsrc=
	std::string altsrc{};// = 2;             // altsrc=
};

struct Status {
	uint32_t battery{};// = 1;           // battery=
};


struct Takv {
	std::string device;// = 1;             // device=
	std::string platform;// = 2;           // platform=
	std::string os;// = 3;                 // os=
	std::string version{};// = 4;            // version=
};

struct Track {
	double speed{};// = 1;           // speed=
	double course{};// = 2;          // course=
};



struct CoTEvent_Detail
{
	st_XMLDetail *st_xmlDetail{};


	std::string xmlDetail{};// = 1;
	Contact *contact{};// = 2;
	Group *group{};// = 3;
	PrecisionLocation *precisionLocation{};// = 4;
	Status *status{};// = 5;
	Takv *takv{};// = 6;
	Track *track{};// = 7;
};

struct TakControl
{
	// Lowest TAK protocol version supported
	// If not filled in (reads as 0), version 1 is assumed
	uint32_t minProtoVersion = 1;

	// Highest TAK protocol version supported
	// If not filled in (reads as 0), version 1 is assumed
	uint32_t maxProtoVersion = 2;

	// UID of the sending contact. May be omitted if
	// this message is paired in a TakMessage with a CotEvent
	// and the CotEvent contains this information
	std::string contactUid{};// = 3;

};

struct CotEvent
{
	int srcIP[4]{};
	int age = 0;
	int count = 1;

	bool isChat{false};

	enum MsgFormat { XML_EVENT, XML_MSG, PROTOBUF };

	enum MsgFormat msgFormat = PROTOBUF;



	std::string type{};               // <event type="x">

	std::string access{};             // optional
	std::string qos{};                // optional
	std::string opex{};               // optional

	std::string uid{};                // <event uid="x">
	uint64_t sendTime{};           // <event time="x"> converted to timeMs
	uint64_t startTime{};          // <event start="x"> converted to timeMs
	uint64_t staleTime{};          // <event stale="x"> converted to timeMs
	std::string s_sendTime{}, s_startTime{}, s_staleTime{};

	std::string how{};                // <event how="x">

	// <point>
	double lat{};               // <point lat="x">
	double lon{};               // <point lon="x">
	double hae{};               // <point hae="x"> use 999999 for unknown
	double ce{};                // <point ce="x"> use 999999 for unknown
	double le{};                // <point ce="x"> use 999999 for unknown

	// comprises children of <detail>
	// This is optional - if omitted, then the cot message
	// had no data under <detail>
	CoTEvent_Detail *detail{};// = 15;

	std::string printTime(uint64_t ms_since_epoch)
	{
		// 1. Create a std::chrono::time_point from milliseconds
		std::chrono::system_clock::time_point tp = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms_since_epoch));

		// 2. Convert time_point to std::time_t
		std::time_t tt = std::chrono::system_clock::to_time_t(tp);

		// 3. Convert std::time_t to std::tm (broken-down time)
		//    Use std::gmtime for UTC, or std::localtime for local time.
		//    Be aware of thread-safety if using std::gmtime/localtime in multi-threaded contexts.
		std::tm* ptm = std::gmtime(&tt); // For UTC time

		char buf[160]{};
		sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", 1900 + ptm->tm_year, 1 + ptm->tm_mon, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
		return std::string(buf);
	};


	void pme()
	{
		if (PROTOBUF==msgFormat)  ImGui::Text("Protbuf");
		else if (XML_EVENT == msgFormat)  ImGui::Text("XML_EVENT");
		else if (XML_MSG == msgFormat)  ImGui::Text("XML_MSG");


		if (nullptr != detail->contact)
		{
			ImGui::Text("COTEvent/Detail/Contact/Call Sign: ");
			ImGui::SameLine();
			ImGui::TextColored(ImColor(255, 0, 0, 255), "%s", detail->contact->callsign.c_str());
		}
		else if (detail->st_xmlDetail->x_callSign.size() != 0)
		{
			ImGui::TextColored(ImColor(255, 0, 0, 255), "COTEvent/Detail/xmlDetail Call Sign: %s", detail->st_xmlDetail->x_callSign.c_str());			
		}
		else ImGui::TextColored(ImColor(255, 0, 0, 255), "No Call Sign in message!");
		
		ImGui::Text("Src IP: %d:%d:%d:%d", srcIP[0], srcIP[1], srcIP[2], srcIP[3]);


		ImGui::Text("COTEvent/UID: %s", uid.c_str() );

		ImGui::Text("COTEvent/Type: %s", type.c_str());
		ImGui::Text("COTEvent/Access: %s", uid.c_str());
		ImGui::Text("COTEvent/QOS: %s", qos.c_str());
		ImGui::Text("COTEvent/OPEX: %s", opex.c_str());
		ImGui::Text("COTEvent/send Time: %s", s_sendTime.c_str());
		ImGui::Text("COTEvent/start Time: %s", s_startTime.c_str());
		ImGui::Text("COTEvent/stale Time: %s", s_staleTime.c_str());
		ImGui::Text("COTEvent/Lat: %7.4f", lat);
		ImGui::Text("COTEvent/Lon: %8.4f", lon);
		ImGui::Text("COTEvent/HAE: %3.1f", hae);
		ImGui::Text("COTEvent/CE: %3.2f", ce);
		ImGui::Text("COTEvent/LE: %3.2f", le);

		if (0 != detail->xmlDetail.size())
		{
			ImGui::TextWrapped("COTEvent/Detail/Contact/xmlDetail: %s", detail->xmlDetail.c_str());
		}



		if (nullptr != detail->contact)
		{
			ImGui::Text("COTEvent/Detail/Contact/endpoint: %s", detail->contact->endpoint.c_str());
			ImGui::Text("COTEvent/Detail/Contact/callsign: %s", detail->contact->callsign.c_str());
		}

		if (nullptr != detail->group)
		{
			ImGui::Text("COTEvent/Detail/Group:name: %s", detail->group->name.c_str());
			ImGui::Text("COTEvent/Detail/Group:role: %s", detail->group->role.c_str());
		}

		if (nullptr != detail->precisionLocation)
		{
			ImGui::Text("COTEvent/Detail/PrecisionLocation:geopointsrc: %s", detail->precisionLocation->geopointsrc.c_str());
			ImGui::Text("COTEvent/Detail/PrecisionLocation:altsrc: %s", detail->precisionLocation->altsrc.c_str());
		}

		if (nullptr != detail->status)
		{
			ImGui::Text("COTEvent/Detail/Status/battery: %d", detail->status->battery);
		}

		if (nullptr != detail->takv)
		{
			ImGui::Text("COTEvent/Detail/Takv/device: %s", detail->takv->device.c_str());
			ImGui::Text("COTEvent/Detail/Takv/platform: %s", detail->takv->platform.c_str());
			ImGui::Text("COTEvent/Detail/Takv/os: %s", detail->takv->os.c_str());
			ImGui::Text("COTEvent/Detail/Takv/version: %s", detail->takv->version.c_str());
		}

		if (nullptr != detail->track)
		{
			ImGui::Text("COTEvent/Detail/Track/speed: %d", detail->track->speed);
			ImGui::Text("COTEvent/Detail/Track/course: %d", detail->track->course);
		}
	}
};




struct TakMessage {
	// Optional - if omitted, continue using last reported control
	// information
	TakControl takControl{};// = 1;

	// Optional - if omitted, no event data in this message
	CotEvent cotEvent{};// = 2;
};



