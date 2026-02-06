#pragma once




#include <chrono>
//#include <wx/log.h>

#include "COT.h"

#include "bg_TakMessage.h"



void bg_TakMessage::AssembleCoTPbufEvent()
{
	PKT.clear();

	//protobuf magic bytes for PBuf CoT
	PKT.push_back(0xbf);
	PKT.push_back(0x01);
	PKT.push_back(0xbf);

	if (IncludeTakControl)
	{
		
		PKT.push_back(0x0A);  
		encodeVarint(0, PKT); //value

		//PKT.push_back(0x08);  
		//encodeVarint(1, PKT); //value
		
		
		//PKT.push_back(0x02);
		//encodeVarint(0, PKT); //length
		//encodeVarint(0, PKT); //length

		
		
		//encodeVarint(0, PKT); //length
	}

	
	int sD = build_CoTEvent();


	PKT.push_back(0x12);  //code for CoTEvent ?
	encodeVarint(sD, PKT);
	for (auto a : CoTEvent) PKT.push_back(a);
}



int bg_TakMessage::build_Detail_Track()
{
	std::vector<unsigned char> tvec;

	if (speed != -99)
	{
		tvec.push_back(0x09);
		insertDouble(tvec, speed);
	}

	if (course != -99)
	{
		tvec.push_back(0x11);
		insertDouble(tvec, course);
	}

	CoTTrack.clear();
	CoTTrack.push_back(0x3A);  //id for track TBC
	CoTTrack.push_back(tvec.size());
	for (auto a : tvec) CoTTrack.push_back(a);

	return CoTTrack.size();
}


int bg_TakMessage::build_Detail_XmlDetail()
{
	std::vector<unsigned char> tvec;

	CoTXmlDetail.clear();
	CoTXmlDetail.push_back(0x0a);
	insertString(CoTXmlDetail, xmlDetail.c_str());

	return CoTXmlDetail.size();
}



int bg_TakMessage::build_Detail_Contact()
{
	std::vector<unsigned char> tvec;

	if (endpoint.size() > 0)  //optional
	{
		tvec.push_back(0x0a);
		insertString(tvec, endpoint.c_str());
	}
	
	tvec.push_back(0x12);
	insertString(tvec, callsign.c_str());

	CoTContact.clear();
	CoTContact.push_back(0x12);  //id for Contact
	CoTContact.push_back(tvec.size());
	for (auto a : tvec) CoTContact.push_back(a);

	return CoTContact.size();
}




int bg_TakMessage::build_Detail_Group()
{
	std::vector<unsigned char> tvec;

	tvec.push_back(0x0a);
	insertString(tvec, group_name.c_str());

	tvec.push_back(0x12);
	insertString(tvec, role.c_str());

	CoTGroup.clear();
	CoTGroup.push_back(0x1a);
	CoTGroup.push_back(tvec.size());
	for (auto a : tvec) CoTGroup.push_back(a);

	return CoTGroup.size();
}


int bg_TakMessage::build_Detail_precisionLocation()
{
	std::vector<unsigned char> tvec;

	if (geopointsrc.size()>0)
	{
		tvec.push_back(0x0a);
		insertString(tvec, geopointsrc.c_str());
	}

	if (altsrc.size()>0)
	{
		tvec.push_back(0x12);
		insertString(tvec, altsrc.c_str());
	}
	
	CoTPrecisionLocation.clear();
	CoTPrecisionLocation.push_back(0x22	);
	CoTPrecisionLocation.push_back(tvec.size());
	for (auto a : tvec) CoTPrecisionLocation.push_back(a);

	return CoTPrecisionLocation.size();
}


int bg_TakMessage::build_Detail_status()
{
	std::vector<unsigned char> tvec;

	tvec.push_back(0x08);
	insertInt(tvec, battery);

	CoTStatus.clear();
	CoTStatus.push_back(0x2a);
	CoTStatus.push_back(tvec.size());
	for (auto a : tvec) CoTStatus.push_back(a);

	return CoTStatus.size();
}

int bg_TakMessage::build_Detail_takv()
{
	std::vector<unsigned char> tvec;

	tvec.push_back(0x0a);
	insertString(tvec, device.c_str());

	tvec.push_back(0x12);
	insertString(tvec, platform.c_str());

	tvec.push_back(0x1a);
	insertString(tvec, os.c_str());

	tvec.push_back(0x22);
	insertString(tvec, version.c_str());

	CoTTakv.clear();
	CoTTakv.push_back(0x32);
	CoTTakv.push_back(tvec.size());
	for (auto a : tvec) CoTTakv.push_back(a);

	return CoTTakv.size();
}


int bg_TakMessage::buildCoTEvent_Detail()
{
	CoTDetail.clear();

	int sXML = 0;
	if (xmlDetail.size()>0) sXML = build_Detail_XmlDetail();

	int sD = 0;
	if (true == includeContact) sD = build_Detail_Contact();
	int sG = 0;
	if (true == includeGroup) sG = build_Detail_Group();

	int sPL = 0;
	if ((geopointsrc.size()>0) || (altsrc.size()>0) ) sPL = build_Detail_precisionLocation();
	
	int sS = 0;
	if (battery !=-1) sS = build_Detail_status();
	int sTV = build_Detail_takv();

	int sTR = 0;
	if ((course !=-99) || (speed !=-99)) sTR = build_Detail_Track();


	CoTDetail.push_back(0x7a);  //this is the detail tag
	int l = sXML + sD + sG + sPL + sS + sTV + sTR;
	encodeVarint(l, CoTDetail);

	for (auto a : CoTXmlDetail) CoTDetail.push_back(a);
	for (auto a : CoTContact) CoTDetail.push_back(a);
	for (auto a : CoTGroup) CoTDetail.push_back(a);
	for (auto a : CoTPrecisionLocation) CoTDetail.push_back(a);
	for (auto a : CoTStatus) CoTDetail.push_back(a);
	for (auto a : CoTTakv) CoTDetail.push_back(a);
	for (auto a : CoTTrack) CoTDetail.push_back(a);

	return CoTDetail.size();
}



int bg_TakMessage::build_CoTEvent()
{
	std::vector<unsigned char> tvec;

	//cotEvent.type
	// tag = 1 <<3
	//wiretype is LEN = 2
	tvec.push_back(0x0a);
	insertString(tvec, msg_type.c_str());



	//cotEvent.access
	// tag = 2 <<3
	//wiretype is LEN = 2
	if (access.size() > 0)
	{
		tvec.push_back(0x12);
		insertString(tvec, access.c_str());
	}
	

	//cotEvent.qos
	// tag = 3 <<3
	//wiretype is LEN = 2
	if (qos.size() > 0)
	{
		tvec.push_back(0x1a);
		insertString(tvec, qos.c_str());
	}

	//cotEvent.opex
	// tag = 4 <<3
	//wiretype is LEN = 2
	if (opex.size() > 1)
	{
		tvec.push_back(0x22);
		insertString(tvec, opex.c_str());
	}

	//cotEvent.uid
	// tag = 5 <<3
	//wiretype is LEN = 2
	tvec.push_back(0x2a);
	insertString(tvec, UID.c_str());

	//Times are number of milliseconds since 1970-01-01 00:00:00 UTC
	auto now = std::chrono::system_clock::now();
	auto duration_since_epoch = now.time_since_epoch();
	auto milliseconds_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(duration_since_epoch).count();

	uint64_t sendTime = (uint64_t)milliseconds_since_epoch;

	std::vector<uint8_t> s_sendTime = encodeVarint64(sendTime);

	tvec.push_back(0x30); //sendTime
	for (int i = 0; i < s_sendTime.size(); i++) tvec.push_back(s_sendTime[i]);

	tvec.push_back(0x38); //startTime  but use send Time
	for (int i = 0; i < s_sendTime.size(); i++) tvec.push_back(s_sendTime[i]);

	uint64_t staleTime = (uint64_t)milliseconds_since_epoch + tm_validTimeInSeconds * 1000;
	std::vector<uint8_t> s_staleTime = encodeVarint64(staleTime);
	tvec.push_back(0x40); //staleTime
	for (int i = 0; i < s_staleTime.size(); i++) tvec.push_back(s_staleTime[i]);

	// cotEvent.how
	tvec.push_back(0x4a);
	insertString(tvec, _how.c_str());

	// cotEvent.lat
	tvec.push_back(0x51);
	insertDouble(tvec, d_lat);

	// cotEvent.lon
	tvec.push_back(0x59);
	insertDouble(tvec, d_lon);

	// cotEvent.hae
	tvec.push_back(0x61);
	insertDouble(tvec, d_hae);

	// cotEvent.ce
	tvec.push_back(0x69);
	insertDouble(tvec, d_ce);

	// cotEvent.le
	tvec.push_back(0x71);
	insertDouble(tvec, d_le);


	CoTEvent.clear();
	for (auto a : tvec) CoTEvent.push_back(a);

	int DetailSize = 0;
	if (includeDetail)
	{
		DetailSize = buildCoTEvent_Detail();
	}

	int EventLength = tvec.size() + DetailSize + 2;
	for (auto a : CoTDetail) CoTEvent.push_back(a);

	return CoTEvent.size();
}







