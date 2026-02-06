
#include <iostream>
#include <conio.h>
#include <vector>
#include <mutex>
#include "SocketStuff.h"
#include "BG_MulticastSocket.h"
//#include "ImGui.h"
//#include "ImBGUtil.h"
#include "COTModule.h"
//#include "ImageDX.h"
///#include "BGUtil.h"
//#include "AppLogger.h"
//#include <imgui_internal.h>

#include "COTSender.h"

#include <inttypes.h> 

#include "pugixml.hpp"

#include <sstream>



void CoTSenderDialog(bool* pOpen);
void CoTSenderLoadIniValues();
void CoTSenderSaveIniValues();

void callbackCOTListener(char* _message, int messageSize, struct sockaddr_in* srcIP);
void CoTMsgLog(bool* pOpen);
void UpdateCOTEntityList(CotEvent* myCotEvent);
st_XMLDetail *ParseXMLDetail(std::string buf);


bg_MulticastSocket* socketGeoChatListen;
char GEOCHAT_MULTICAST_LISTEN_GROUP[20] = "224.10.10.1";
int GEOCHAT_MULTICAST_LISTEN_PORT = 17012;
bool isGEOCHATListenerRunning = false;



bg_MulticastSocket* socketCOTListen;
char COT_MULTICAST_LISTEN_GROUP[20] = "239.2.3.1";
int COT_MULTICAST_LISTEN_PORT = 6969;
bool isCOTListenerRunning = false;

bool isCOTSimRunning = false;
bool b_COTParseError = false;
extern int numParseFails;
int COTMsgCount = 0;
int GeoChatMsgCount = 0;
int PLICount = 0;
int ProtobufCount = 0;
int XMLCount = 0;
int parseErrorCount = 0;

extern char COT_MULTICAST_SEND_GROUP[20];
extern int COT_MULTICAST_SEND_PORT;
extern bool isCOTSenderRunning;
extern int K5_1_StaleTime;

static HANDLE ptrTimerHandle;
static void __stdcall CoTTimerCallback(PVOID, BOOLEAN);//(PVOID lpParameter, BOOLEAN TimerOrWaitFired)

void callbackGeoChatListener(char* _message, int messageSize, struct sockaddr_in* _srcIP);


std::mutex COTMsgList_mtx;
std::vector<CotEvent*> CotEventList;

std::mutex COTEntityList_mtx;
std::vector<CotEvent*> CotEntityList;

std::mutex COTChatList_mtx;
std::vector<CotEvent*> CotChatList;



std::vector <TakControl*> TakControlList;

bool b_ShowLatestCOTMessage = true;
bool mb_ShowCOTPLIEntityList = true;

struct sockaddr_in* srcIP;

bool filter_Type_a = false;
bool filter_Type_t = false;
bool filter_Type_x = false;

#pragma region COTListener

extern bool isCOTSenderRunning;

static int InitMcastListenerCOT(char* MCGroup, int MCPort)
{
	if (isCOTListenerRunning) return 0;
	if (socketCOTListen != NULL) return 0;

	//isCOTListenerRunning = false;  //this is implied
	socketCOTListen = new bg_MulticastSocket();

	const unsigned short LOOP_BACK = 1;
	const unsigned short TTL = 1;
	bool result = socketCOTListen->Intialize(MCGroup, MCPort, LOOP_BACK, TTL, &callbackCOTListener);
	if (result == false)
	{
		std::cout << "Failed to initialize multicast socket." << std::endl;
		return 1;
	}
	else if (true) puts("Opened COT Listener");
	printf("Listening on %s:%d\r\n", MCGroup, MCPort);
	isCOTListenerRunning = true;
	return 0;
}

static int InitMcastListenerGeoChat(char* MCGroup, int MCPort)
{
	if (isGEOCHATListenerRunning) return 0;
	if (socketGeoChatListen != NULL) return 0;

	//isGEOCHATListenerRunning = false;  //this is implied
	socketGeoChatListen = new bg_MulticastSocket();
	
	const unsigned short LOOP_BACK = 1;
	const unsigned short TTL = 1;
	bool result = socketGeoChatListen->Intialize(MCGroup, MCPort, LOOP_BACK, TTL, &callbackGeoChatListener);
	
	if (result == false)
	{
		std::cout << "Failed to initialize multicast socket." << std::endl;
		return 1;
	}
	else if (true) puts("Opened Geo Chat Listener");
	printf("Listening on %s:%d\r\n", MCGroup, MCPort);
	isGEOCHATListenerRunning = true;
	return 0;
}



struct PB_KLV
{
	//protobuf TAG/KEY is fieldNumber and wiretype plus varint continuation bit
	int m_fieldNumber{}; // 5 bits so max 32 field numbers if we don't use the continuation bit. We won't use it for CoT
	int m_wireType{}; //lower 3 bytes of TAG
		
	uint64_t m_Varint{};	//value wiretype is VARINT
	uint32_t m_I32{};
	float m_fI32{};	//value if wiretype is I32 

	uint64_t m_I64{};
	double m_dI64{};	//value if wiretype is I64 


	std::string m_text{}; //value if wiretype is LEN

	int m_lengthOfVarint{}; //varint encoded length of value in KLV
	int m_sizeOfKLV{}; //length of Key and Length parts if KLV, used to index to next enclosed KLV
	int m_sizeOfKL{};
	int m_payLoadLength{};

	const char* wireTypeStr[6] = { "VARINT(0)","I64(1)","LEN(2)","SGROUP(3)","EGROUP(4)","I32(5)" };
		/*
		0	VARINT	int32, int64, uint32, uint64, sint32, sint64, bool, enum
		1	I64	fixed64, sfixed64, double
		2	LEN	string, bytes, embedded messages, packed repeated fields
		3	SGROUP	group start (deprecated)
		4	EGROUP	group end (deprecated)
		5	I32	fixed32, sfixed32, float
		*/

	const char* TakControlTags[4] = { "zero","minProtoVersion","maxProtoVersion","contactUid"};
	const char* eventTags[16] = { "zero","type","access","qos","opex","uid","sendTime","startTime","staleTime","how","lat","lng","hae","ce","le","detail" };

	const char* Detail[8] = { "zero","xmlDetail","contact","group","precisionLocation","status","takv","track"};
	const char* Contact[3] = { "zero","endpoint","callsign"};
	const char* Group[3] = { "zero","name","role" };
	
	const char* PrecisionLocation[3] = { "zero","geopointsrc","altsrc"};
	const char* Status[2] = { "zero","battery"};


	const char* Takv[5] = { "zero","device","platform","os","version"};
	const char* Track[3] = { "zero","speed","course"};



	static int getNextWireType(uint8_t *buf)
	{
		return (buf[0] & 0b00000111);
	}

	static int getNextFieldNumber(uint8_t* buf)
	{
		return (buf[0] & 0b01111000) >> 3;
	}
	
	
	PB_KLV(uint8_t* buf, bool parseText = true)
	{
		printf("%02X %02X %02X\r\n", (unsigned char)buf[0], (unsigned char)buf[1], (unsigned char)buf[2]);
		
		m_fieldNumber = getNextFieldNumber(buf);
		m_wireType = getNextWireType(buf);

		std::string m_ValueStr = "Not Parsed";
		switch (m_wireType)
		{
			case 0: //VARINT  . this includes the three time stamps
			{
				m_Varint = decodeVarint(&buf[1], m_lengthOfVarint);
				m_sizeOfKLV = 1 + m_lengthOfVarint;
				m_sizeOfKL = 1;
				m_ValueStr = std::to_string(m_Varint);
				//putsYellow("%02X%02X%02X%02X%02X%02X   %u", buf[1],buf[2], buf[3], buf[4], buf[5], buf[6], m_Varint);
				break;
			}
			case 1: //I64
			{
				m_sizeOfKLV = 1 + 8;
				m_sizeOfKL = 1;

				m_dI64 = decodeI64asDouble(&buf[1]);
				m_I64 = decodeI64(&buf[1]);
				break;
			}
			case 2: //LEN
			{
				m_payLoadLength = decodeVarint(&buf[1], m_lengthOfVarint);
				m_sizeOfKLV = 1 + m_lengthOfVarint + m_payLoadLength;	//add 1 to account for the TAG/KEY. This would change if we allowed more than 32 fieldNumbers...which we don't for CoT
				m_sizeOfKL = 1 + m_lengthOfVarint;
				if (parseText)
				{
					m_text = std::string((char*)&buf[1 + m_lengthOfVarint], m_payLoadLength);
					m_ValueStr = m_text;
				}
				break;
			}
			case 5: //I32
			{
				m_sizeOfKLV = 1 + 4;
				m_sizeOfKL = 1;

				m_fI32 = decodeI32asFloat(&buf[1]);
				m_I32 = decodeI32(&buf[1]);

				m_ValueStr = std::to_string(m_I32) + " or " + std::to_string(m_fI32);
				break;
			}
		} //end switch

		printf("Field Number: %d  WireType: %s  length %d, size of KL: %d value: %s\r\n", m_fieldNumber, wireTypeStr[m_wireType], m_payLoadLength, m_sizeOfKLV, m_ValueStr.c_str());
	};



	// Function to decode a varint from a byte buffer
	uint64_t decodeVarint(uint8_t* data, int& lengthOfVarint) {
		uint64_t value = 0;
		int index = 0;
		uint8_t byte;

		//putsRed("Decode varint:");
		do {
			byte = data[index];
			uint64_t a = static_cast<uint64_t>(byte & 0x7F);
			uint64_t b = static_cast<uint64_t>(index * 7);
			uint64_t t = a << b;
			value += t;
			//printf("%02X %" PRIu64 " %" PRIu64 " %" PRIu64 " \r\n", byte, a, b , t);
			index++;
		} while ((byte & 0x80) != 0); // Continue if MSB is set

		lengthOfVarint = index;
		//putsRed("\r\ndecodeVarint: Varint is %" PRIu64,value);
		return value;
	};

	
	double decodeI64asDouble(uint8_t* data)  //decoded as double
	{
		double copied_double;
		std::memcpy(&copied_double, data, sizeof(double));
		return copied_double;
	}


	uint64_t decodeI64(uint8_t* data)  //decoded as double
	{
		uint64_t retVal{};
		std::memcpy(&retVal, data, sizeof(uint64_t));
		return retVal;
	}


	uint32_t decodeI32(uint8_t* data)
	{
		uint32_t retVal{};
		std::memcpy(&retVal, data, sizeof(uint32_t));
		return retVal;
	}

	float decodeI32asFloat(uint8_t* data)
	{
		float copied_double;
		std::memcpy(&copied_double, data, sizeof(float));
		return copied_double;
	}

};

std::pair<int, Track*> parseTrack(uint8_t* buf, int bufLength)
{
	putsGreen("In parseTrack with bufLength (including Track KL) %d", bufLength);
	PB_KLV KLVofTrack(buf, false);

	if (0 == KLVofTrack.m_payLoadLength)
	{
		putsRed("CoT PrecisionLocation has no fields");
		return { KLVofTrack.m_sizeOfKLV, nullptr };
	}

	Track* myTrack = new Track();

	int index = KLVofTrack.m_sizeOfKL;
	for (int t = 0;t < 2;t++)
	{
		int FieldNumber = PB_KLV::getNextFieldNumber(buf + index);
		putsGreen("\t\tParsing CoT Detail.group.%d %s", FieldNumber, KLVofTrack.Track[FieldNumber]);

		switch (FieldNumber)
		{
		case 1:
		{
			PB_KLV parseTrackItem(buf + index);
			index += parseTrackItem.m_sizeOfKLV;
			myTrack->speed = parseTrackItem.m_dI64;
			break;
		}
		case 2:
		{
			PB_KLV parseTrackItem(buf + index);
			index += parseTrackItem.m_sizeOfKLV;
			myTrack->course = parseTrackItem.m_dI64;
			break;
		}

		}
		if (index >= KLVofTrack.m_payLoadLength) break;
	};

	putsRed("\t\tGroup: Parsed %d bytes of %d, %d bytes remaining", index, 2 + KLVofTrack.m_payLoadLength, bufLength - index);



	return { index, myTrack };
}


std::pair<int, Takv*> parseTakv(uint8_t* buf, int bufLength)
{
	putsGreen("In parseGroup with bufLength (including Contact KL) %d", bufLength);
	PB_KLV KLVofTakv(buf, false);

	if (0 == KLVofTakv.m_payLoadLength)
	{
		putsRed("CoT Contact has no fields");
		return { KLVofTakv.m_sizeOfKLV, nullptr };
	}

	Takv* myTakv = new Takv();

	int index = KLVofTakv.m_sizeOfKL;
	for (int t = 0;t < 4;t++)
	{
		int FieldNumber = PB_KLV::getNextFieldNumber(buf + index);
		putsGreen("\t\tParsing CoT Detail.Takv.%d %s", FieldNumber, KLVofTakv.Takv[FieldNumber]);

		switch (FieldNumber)
		{
		case 1:
		{
			PB_KLV parseTakvItem(buf + index);
			index += parseTakvItem.m_sizeOfKLV;
			myTakv->device = parseTakvItem.m_text;
			break;
		}
		case 2:
		{
			PB_KLV parseTakvItem(buf + index);
			index += parseTakvItem.m_sizeOfKLV;
			myTakv->platform = parseTakvItem.m_text;
			break;
		}
		case 3:
		{
			PB_KLV parseTakvItem(buf + index);
			index += parseTakvItem.m_sizeOfKLV;
			myTakv->os = parseTakvItem.m_text;
			break;
		}
		case 4:
		{
			PB_KLV parseTakvItem(buf + index);
			index += parseTakvItem.m_sizeOfKLV;
			myTakv->version = parseTakvItem.m_text;
			break;
		}

		}
		if (index >= KLVofTakv.m_payLoadLength) break;
	};

	putsRed("\t\tTakv: Parsed %d bytes of %d, %d bytes remaining", index, 2 + KLVofTakv.m_payLoadLength, bufLength - index);

	return { index, myTakv };
}



std::pair<int, Status*> parseStatus(uint8_t* buf, int bufLength)
{
	putsGreen("In parseStatus with bufLength (including KL) %d", bufLength);
	PB_KLV KLVofStatus(buf, false);

	if (0 == KLVofStatus.m_payLoadLength)
	{
		putsRed("CoT Status has no fields");
		return { KLVofStatus.m_sizeOfKLV, nullptr };
	}

	Status* myStatus = new Status();

	int index = KLVofStatus.m_sizeOfKL;
	for (int t = 0;t < 2;t++)
	{
		int FieldNumber = PB_KLV::getNextFieldNumber(buf + index);
		putsGreen("\t\tParsing CoT Detail.group.%d %s", FieldNumber, KLVofStatus.Status[FieldNumber]);

		switch (FieldNumber)
		{
		case 1:
		{
			PB_KLV parseStatus(buf + index);
			index += parseStatus.m_sizeOfKLV;
			myStatus->battery = parseStatus.m_I32;
			break;
		}
		}
		if (index >= KLVofStatus.m_payLoadLength) break;
	};

	putsRed("\t\tGroup: Parsed %d bytes of %d, %d bytes remaining", index, 2 + KLVofStatus.m_payLoadLength, bufLength - index);



	return { index, myStatus };
}


std::pair<int, PrecisionLocation*> parsePrecisionLocation(uint8_t* buf, int bufLength)
{
	putsGreen("In parsePrecisionLocation with bufLength (including Contact KL) %d", bufLength);
	PB_KLV KLVofPrecisionLocation(buf, false);

	if (0 == KLVofPrecisionLocation.m_payLoadLength)
	{
		putsRed("CoT PrecisionLocation has no fields");
		return { KLVofPrecisionLocation.m_sizeOfKLV, nullptr };
	}

	PrecisionLocation* myPrecisionLocation = new PrecisionLocation();

	int index = KLVofPrecisionLocation.m_sizeOfKL;
	for (int t = 0;t < 2;t++)
	{
		int FieldNumber = PB_KLV::getNextFieldNumber(buf + index);
		putsGreen("\t\tParsing CoT Detail.group.%d %s", FieldNumber, KLVofPrecisionLocation.PrecisionLocation[FieldNumber]);

		switch (FieldNumber)
		{
		case 1:
		{
			PB_KLV parseContactItem(buf + index);
			index += parseContactItem.m_sizeOfKLV;
			myPrecisionLocation->geopointsrc = parseContactItem.m_text;
			break;
		}
		case 2:
		{
			PB_KLV parseContactItem(buf + index);
			index += parseContactItem.m_sizeOfKLV;
			myPrecisionLocation->altsrc = parseContactItem.m_text;
			break;
		}

		}
		if (index >= KLVofPrecisionLocation.m_payLoadLength) break;
	};

	putsRed("\t\tGroup: Parsed %d bytes of %d, %d bytes remaining", index, 2 + KLVofPrecisionLocation.m_payLoadLength, bufLength - index);



	return { index, myPrecisionLocation };
}

std::pair<int, Group*> parseGroup(uint8_t* buf, int bufLength)
{
	putsGreen("In parseGroup with bufLength (including Contact KL) %d", bufLength);
	PB_KLV KLVofGroup(buf, false);

	if (0 == KLVofGroup.m_payLoadLength)
	{
		putsRed("CoT Contact has no fields");
		return { KLVofGroup.m_sizeOfKLV, nullptr };
	}

	Group* myGroup = new Group();

	int index = KLVofGroup.m_sizeOfKL;
	for (int t = 0;t < 2;t++)
	{
		int FieldNumber = PB_KLV::getNextFieldNumber(buf + index);
		putsGreen("\t\tParsing CoT Detail.group.%d %s", FieldNumber, KLVofGroup.Group[FieldNumber]);

		switch (FieldNumber)
		{
		case 1:  
		{
			PB_KLV parseContactItem(buf + index);
			index += parseContactItem.m_sizeOfKLV;
			myGroup->name = parseContactItem.m_text;
			break;
		}
		case 2: 
		{
			PB_KLV parseContactItem(buf + index);
			index += parseContactItem.m_sizeOfKLV;
			myGroup->role = parseContactItem.m_text;
			break;
		}

		}
		if (index >= KLVofGroup.m_payLoadLength) break;
	};

	putsRed("\t\tGroup: Parsed %d bytes of %d, %d bytes remaining", index, 2 + KLVofGroup.m_payLoadLength, bufLength - index);



	return { index, myGroup };
}


std::pair<int, Contact*> parseContact(uint8_t* buf, int bufLength)
{
	putsGreen("In parseContact with bufLength (including Contact KL) %d", bufLength);
	PB_KLV KLVofContact(buf, false);

	if (0 == KLVofContact.m_payLoadLength)
	{
		putsRed("CoT Contact has no fields");
		return { KLVofContact.m_sizeOfKLV, nullptr};
	}

	Contact* myContact = new Contact();

	int index = KLVofContact.m_sizeOfKL;
	for (int t=0;t<2;t++) //limit loop to 2 passes
	{
		int FieldNumber = PB_KLV::getNextFieldNumber(buf + index);
		putsGreen("\t\tParsing CoT Detail.contact.%s", KLVofContact.Contact[FieldNumber]);

		switch (FieldNumber)
		{
		case 1:  
		{
			PB_KLV parseContactItem(buf + index);
			index += parseContactItem.m_sizeOfKLV;
			myContact->endpoint = parseContactItem.m_text;
			break;
		}
		case 2: 
		{
			PB_KLV parseContactItem(buf + index);
			index += parseContactItem.m_sizeOfKLV;
			myContact->callsign = parseContactItem.m_text;
			break;
		}
		}
		if (index >= KLVofContact.m_payLoadLength) break;
	} 

	putsRed("\t\tContact: Parsed %d bytes of %d, %d bytes remaining", index, 2 + KLVofContact.m_payLoadLength, bufLength - index);


	return {index, myContact };
}

std::pair<int, CoTEvent_Detail*>   parseDetail(uint8_t* buf, int bufLength)
{
	putsBlue("\tIn parseDetail with bufLength (including Detail KL) %d", bufLength);
		
	PB_KLV KLVofDetail(buf, false);

	if (0 == KLVofDetail.m_payLoadLength)
	{
		putsRed("CoT Detail has no fields");
		return { KLVofDetail.m_sizeOfKLV, nullptr };
	}
	

	CoTEvent_Detail* myCoTEvent_Detail = new CoTEvent_Detail();

	int t = 5;
	int index = KLVofDetail.m_sizeOfKL;
	for (int t = 0;t < 7;t++)  //limit loop to 7 passes
	{
		int FieldNumber = PB_KLV::getNextFieldNumber(buf + index);
		putsBlue("\tParsing CoT Detail.%s", KLVofDetail.Detail[FieldNumber]);

		switch (FieldNumber) 
		{
			case 1: 
			{
				PB_KLV parseDetailItem(buf + index);
				index += parseDetailItem.m_sizeOfKLV;
				myCoTEvent_Detail->xmlDetail = parseDetailItem.m_text;
				myCoTEvent_Detail->st_xmlDetail = ParseXMLDetail(myCoTEvent_Detail->xmlDetail);
				break;
			}
			case 2: //contact
			{
				//int consumed = parseContact(buf + index, bufLength - index);
				std::pair<int, Contact *> retPair = parseContact(buf + index, bufLength - index);
				index += retPair.first;
				myCoTEvent_Detail->contact = retPair.second;
				break;
			}
			case 3: //group
			{
				std::pair<int, Group*> retPair = parseGroup(buf + index, bufLength - index);
				index += retPair.first;
				myCoTEvent_Detail->group = retPair.second;
				break;
			}
			case 4: //precisionlocation
			{
				std::pair<int, PrecisionLocation*> retPair = parsePrecisionLocation(buf + index, bufLength - index);
				index += retPair.first;
				myCoTEvent_Detail->precisionLocation = retPair.second;
				break;
			}
			case 5: //status
			{
				std::pair<int, Status*> retPair = parseStatus(buf + index, bufLength - index);
				index += retPair.first;
				myCoTEvent_Detail->status = retPair.second;
				break;
			}
			case 6: //takv
			{
				std::pair<int, Takv*> retPair = parseTakv(buf + index, bufLength - index);
				index += retPair.first;
				myCoTEvent_Detail->takv = retPair.second;
				break;
			}
			case 7: //track
			{
				std::pair<int, Track*> retPair = parseTrack(buf + index, bufLength - index);
				index += retPair.first;
				myCoTEvent_Detail->track = retPair.second;
				break;
			}

		}
			if (index >= (bufLength- KLVofDetail.m_sizeOfKL)) break;
	};// while (true);

	putsRed("\tDetail: Parsed %d bytes, %d bytes remaining", index, bufLength - index);


	return{ index, myCoTEvent_Detail };
}


int  parseCoTEvent(uint8_t*buf, int bufLength)
{
	putsGreen("In parseCoTEvent with bufLength (including CoTEvent KL) %d", bufLength);
	
	PB_KLV KLVofCoTEvent(buf, false);


	if (0 == KLVofCoTEvent.m_payLoadLength)
	{
		putsRed("CoTEvent has no fields");
		return KLVofCoTEvent.m_sizeOfKLV;
	}

	CotEvent* myCotEvent = new CotEvent();
	myCotEvent->msgFormat = CotEvent::MsgFormat::PROTOBUF;


	myCotEvent->srcIP[0] = srcIP->sin_addr.S_un.S_un_b.s_b1;
	myCotEvent->srcIP[1] = srcIP->sin_addr.S_un.S_un_b.s_b2;
	myCotEvent->srcIP[2] = srcIP->sin_addr.S_un.S_un_b.s_b3;
	myCotEvent->srcIP[3] = srcIP->sin_addr.S_un.S_un_b.s_b4;

	int index = KLVofCoTEvent.m_sizeOfKL;

	for (int t = 0;t < 15;t++) //allow max of 15 loops
	{
		int FieldNumber = PB_KLV::getNextFieldNumber(buf + index);
		putsGreen("Parsing CoT Event.%s", KLVofCoTEvent.eventTags[FieldNumber]);

		switch (FieldNumber) 
		{
		case 1:
		{
			PB_KLV eventItem(buf + index);
			index += eventItem.m_sizeOfKLV;
			myCotEvent->type = eventItem.m_text;
			break;
		}
		case 2:
		{
			PB_KLV eventItem(buf + index);
			index += eventItem.m_sizeOfKLV;
			myCotEvent->access = eventItem.m_text;
			break;
		}
		case 3:
		{
			PB_KLV eventItem(buf + index);
			index += eventItem.m_sizeOfKLV;
			myCotEvent->qos = eventItem.m_text;
			break;
		}
		case 4:
		{
			PB_KLV eventItem(buf + index);
			index += eventItem.m_sizeOfKLV;
			myCotEvent->opex = eventItem.m_text;
			break;
		}
		case 5:
		{
			PB_KLV eventItem(buf + index);
			index += eventItem.m_sizeOfKLV;
			myCotEvent->uid = eventItem.m_text;
			break;
		}
		case 6:
		{
			PB_KLV eventItem(buf + index);
			index += eventItem.m_sizeOfKLV;
			myCotEvent->sendTime = eventItem.m_Varint;
			//putsRed("CoTEvent parse sendTime: %" PRIu64 , eventItem.m_Varint);
			myCotEvent->s_sendTime = myCotEvent->printTime(eventItem.m_Varint);
			break;
		}
		case 7:
		{
			PB_KLV eventItem(buf + index);
			index += eventItem.m_sizeOfKLV;
			myCotEvent->startTime = eventItem.m_Varint;
			myCotEvent->s_startTime = myCotEvent->printTime(eventItem.m_Varint);
			break;
		}
		case 8:
		{
			PB_KLV eventItem(buf + index);
			index += eventItem.m_sizeOfKLV;
			myCotEvent->staleTime = eventItem.m_Varint;
			myCotEvent->s_staleTime = myCotEvent->printTime(eventItem.m_Varint);
			break;
		}
		case 9:
		{
			PB_KLV eventItem(buf + index);
			index += eventItem.m_sizeOfKLV;
			myCotEvent->how = eventItem.m_text;
			break;
		}
		case 10:
		{
			PB_KLV eventItem(buf + index);
			index += eventItem.m_sizeOfKLV;
			myCotEvent->lat = eventItem.m_dI64;
			break;
		}
		case 11:
		{
			PB_KLV eventItem(buf + index);
			index += eventItem.m_sizeOfKLV;
			myCotEvent->lon = eventItem.m_dI64;
			break;
		}
		case 12:
		{
			PB_KLV eventItem(buf + index);
			index += eventItem.m_sizeOfKLV;
			myCotEvent->hae = eventItem.m_dI64;
			break;
		}
		case 13:
		{
			PB_KLV eventItem(buf + index);
			index += eventItem.m_sizeOfKLV;
			myCotEvent->ce = eventItem.m_dI64;
			break;
		}
		case 14:
		{
			PB_KLV eventItem(buf + index);
			index += eventItem.m_sizeOfKLV;
			myCotEvent->le = eventItem.m_dI64;
			break;
		}

		case 15:
		{
			std::pair<int, CoTEvent_Detail*> retPair = parseDetail(buf + index, bufLength - index);
			index += retPair.first;
			myCotEvent->detail = retPair.second;

			if (nullptr != myCotEvent->detail->st_xmlDetail)
			{
				if (0 != myCotEvent->detail->st_xmlDetail->chatRoom.size())
					myCotEvent->isChat = true;
			}
			break;
		}
		}
		if (index >= bufLength) break;
	};
		
	putsRed("CoTEvent: Parsed %d bytes, %d bytes remaining", index, bufLength - index);


	std::lock_guard<std::mutex> lock(COTMsgList_mtx);
	CotEventList.push_back(myCotEvent);

	if (myCotEvent->isChat)
		CotChatList.push_back(myCotEvent);
	else
		UpdateCOTEntityList(myCotEvent);
	

	return index;
}

void UpdateCOTEntityList(CotEvent *myCotEvent)
{
	std::lock_guard<std::mutex> lock(COTEntityList_mtx);
	
	for (auto e : CotEntityList)
	{
		if (e->uid == myCotEvent->uid)
		{

			putsRed("%s",e->uid.c_str());
			putsYellow("%s", myCotEvent->uid.c_str());
			e->lat = myCotEvent->lat;
			e->lon = myCotEvent->lon;
			e->hae = myCotEvent->hae;
			e->how = myCotEvent->how;
			e->ce = myCotEvent->ce;
			e->le = myCotEvent->le;
			e->sendTime = myCotEvent->sendTime;
			e->s_sendTime = myCotEvent->s_sendTime;
			e->startTime = myCotEvent->startTime;
			e->s_startTime = myCotEvent->s_startTime;
			e->staleTime = myCotEvent->staleTime;
			e->s_staleTime = myCotEvent->s_staleTime;

			e->srcIP[0] = myCotEvent->srcIP[0];
			e->srcIP[1] = myCotEvent->srcIP[1];
			e->srcIP[2] = myCotEvent->srcIP[2];
			e->srcIP[3] = myCotEvent->srcIP[3];

			e->age = 0;

			e->count++;


			return;
		}
	}

	//it's new so add it
	CotEntityList.push_back(myCotEvent);


}




int  parseTakControl(uint8_t* buf, int bufLength)
{
	putsGreen("In parseTakControl with bufLength (including TakControl KL) %d", bufLength);
	PB_KLV KLVofTakControl(buf, false);
		
	if (0 == KLVofTakControl.m_payLoadLength) 
	{
		putsRed("TakControl has no fields");
		return KLVofTakControl.m_sizeOfKLV;
	}

	TakControl* myTakControl = new TakControl();

	int index = KLVofTakControl.m_sizeOfKL;
	for (int t = 0;t < 3;t++) //allow max of 3 loops
	{
		int FieldNumber = PB_KLV::getNextFieldNumber(buf + index);
		putsGreen("TakControlItem.%s\r\n", KLVofTakControl.TakControlTags[FieldNumber]);

		switch (FieldNumber)
		{
		case 1:
		{
			PB_KLV TakControlItem(buf + index);
			index += TakControlItem.m_sizeOfKLV;
			myTakControl->minProtoVersion = TakControlItem.m_I32;
			break;
		}
		case 2:
		{
			PB_KLV TakControlItem(buf + index);
			index += TakControlItem.m_sizeOfKLV;
			myTakControl->maxProtoVersion = TakControlItem.m_I32;
			break;
		}
		case 3:
		{
			PB_KLV TakControlItem(buf + index);
			index += TakControlItem.m_sizeOfKLV;
			myTakControl->contactUid = TakControlItem.m_text;
			break;
		}
		}
		putsYellow("***index: %d  bufLength: %d", index, KLVofTakControl.m_sizeOfKLV);
		if (index >= KLVofTakControl.m_sizeOfKLV) break;
	};

	TakControlList.push_back(myTakControl);

	putsRed("TakControl: Parsed %d bytes, %d bytes remaining", index, bufLength - index);

	return index;
}





void ParseProtobuf(uint8_t *buf, int bufLength)
{
	int index = 0;
	for (int t = 0;t < 2;t++)  //limit loop to 2 passes max
	{
		int FieldNumber = PB_KLV::getNextFieldNumber(buf+index);
		putsYellow("Pass %d for field: %d\r\n", t, FieldNumber);
		switch (FieldNumber)
		{
		case 1:
		{
			putsRed("Found TakControl");
			index += parseTakControl(buf+index, bufLength-index);
			break;
		}
		case 2:
		{
			putsRed("Found CotEvent");
			index += parseCoTEvent(buf+index, bufLength-index);
			break;
		}
		default:
			putsRed("Found Crap");

		}//end switch
		if (index >= bufLength) break;
	}
	putsRed("Parsed %d bytes, %d bytes remaining", index, bufLength - index);
	
}

void ParseXMLCoT(char* buf, int bufLength)
{
	CotEvent* myCE = new CotEvent();
	myCE->msgFormat = CotEvent::MsgFormat::XML_EVENT;

	myCE->srcIP[0] = srcIP->sin_addr.S_un.S_un_b.s_b1;
	myCE->srcIP[1] = srcIP->sin_addr.S_un.S_un_b.s_b2;
	myCE->srcIP[2] = srcIP->sin_addr.S_un.S_un_b.s_b3;
	myCE->srcIP[3] = srcIP->sin_addr.S_un.S_un_b.s_b4;


	pugi::xml_document doc_read;
	pugi::xml_parse_result result = doc_read.load_buffer_inplace(buf, bufLength);
	if (!result) {
		AddLog("Error loading XML file: %s", result.description());
		return ;
	}

	//AddLog("\r\nCoT");
	for (pugi::xml_node event : doc_read.children("event")) {
		std::string version = event.attribute("version").value();
		myCE->uid = event.attribute("uid").value();
		myCE->type = event.attribute("type").value();
		myCE->sendTime = event.attribute("time").as_double();
		myCE->s_sendTime = myCE->printTime(myCE->sendTime);

		myCE->startTime = event.attribute("start").as_double();
		myCE->s_startTime = myCE->printTime(myCE->sendTime);
		myCE->staleTime = event.attribute("stale").as_double();
		myCE->s_staleTime = myCE->printTime(myCE->sendTime);
		myCE->how = event.attribute("how").value();
		
		//AddLog("version is %s", version.c_str());
		//AddLog("uid is %s", myCE->uid.c_str());
		//AddLog("type is %s", myCE->type.c_str());

	}

	myCE->detail = new CoTEvent_Detail();

	for (pugi::xml_node point : doc_read.child("event").children("point")) 
	{
		myCE->lat = point.attribute("lat").as_double();
		myCE->lon = point.attribute("lon").as_double();
		myCE->hae = point.attribute("hae").as_double();
		myCE->ce= point.attribute("ce").as_double();
		myCE->le = point.attribute("le").as_double();
		
		//AddLog("Lat is %7.4f", myCE->lat);
		//AddLog("Lon is %8.4f", myCE->lon);
		//AddLog("HAE is %3.2f", myCE->hae);
		//AddLog("ce is %3.2f", myCE->ce);
		//AddLog("le is %3.2f", myCE->le);
		
	}

	myCE->detail->contact = new Contact();
	for (pugi::xml_node contact : doc_read.child("event").child("detail").children("contact"))
	{
		myCE->detail->contact->callsign = contact.attribute("callsign").value();
		myCE->detail->contact->endpoint = contact.attribute("endpoint").value();
		//AddLog("callsign is %s", myCE->detail->contact->callsign.c_str());
		//AddLog("endpoint is %s", myCE->detail->contact->endpoint.c_str());
	}

	myCE->detail->track = new Track();
	for (pugi::xml_node track : doc_read.child("event").child("detail").children("track"))
	{
		myCE->detail->track->speed = track.attribute("speed").as_double();
		myCE->detail->track->course = track.attribute("course").as_double();
		//AddLog("speed is %3.2f", myCE->detail->track->speed);
		//AddLog("course is %3.2f", myCE->detail->track->course);
	}


	UpdateCOTEntityList(myCE);

	std::lock_guard<std::mutex> lock(COTMsgList_mtx);
	CotEventList.push_back(myCE);


}


st_XMLDetail *ParseXMLDetail(std::string buf)
{
	st_XMLDetail *myX = new st_XMLDetail();

	putsGreen(buf);

	pugi::xml_document doc_read;
	pugi::xml_parse_result result = doc_read.load_buffer_inplace((void *)buf.c_str(), buf.size());
	
	/*
	if (!result) {
		AddLog("Error loading XMLDetail buffer: %s", result.description());
		return nullptr;
	};
	*/

	if (result)
	{
		std::cout << "XML [" << buf << "] parsed without errors, attr value: [" << doc_read.child("node").attribute("attr").value() << "]\n\n";
	}
	else
	{
		std::cout << "XML parsed with errors, attr value: [" << doc_read.child("node").attribute("attr").value() << "]\n";
		std::cout << "Error description: " << result.description() << "\n";
		std::cout << "Error offset: " << result.offset << " (error at [..." << (buf.c_str() + result.offset) << "]\n\n";
		return nullptr;
	}



	AddLog("\r\nCoT-xmlDetail");
	
	for (pugi::xml_node contact : doc_read.children("contact")) {
		myX->x_uid = contact.attribute("uid").value();
		myX->x_callSign = contact.attribute("callsign").value();
	}

	for (pugi::xml_node chat : doc_read.children("__chat")) {
		AddLog("parsing chat");
		myX->parent = chat.attribute("parent").value();
		myX->groupOwner = chat.attribute("groupOwner").value();
		myX->messageId = chat.attribute("messageId").value();
		myX->chatRoom = chat.attribute("chatroom").value();
		myX->id = chat.attribute("id").value();
		myX->senderCallSign = chat.attribute("senderCallsign").value();
		myX->chatgrp_uid0 = chat.attribute("chatgrp_uid0").value();
		myX->chatgrp_uid1 = chat.attribute("chatgrp_uid1").value();
		myX->chatgrp_id = chat.attribute("chatgrp_id").value();
		putsRed("__chat. chatRoom:%s  senderCallsign:%s", myX->chatRoom.c_str(), myX->senderCallSign.c_str());
	}

	for (pugi::xml_node link : doc_read.children("link")) {
		myX->link_uid = link.attribute("uid").value();
		myX->link_type = link.attribute("type").value();
		myX->link_relation = link.attribute("relation").value();
		putsRed("link. uid:%s  type:%s", myX->link_uid.c_str(), myX->link_type.c_str());
	}

	for (pugi::xml_node remarks : doc_read.children("remarks")) {
		myX->remarks_source = remarks.attribute("source").value();
		myX->remarks_to = remarks.attribute("to").value();
		putsRed("remarks. source:%s  to:%s", myX->remarks_source.c_str(), myX->remarks_to.c_str());
		
		myX->remarks_time = remarks.attribute("time").value();
		putsRed("remarks. time:%s", myX->remarks_time.c_str());

		myX->remarks_text = remarks.child_value();
		putsGreen("remarks. text:%s", myX->remarks_text.c_str());
	}

	
	return myX;
}


void callbackCOTListener(char* _message, int messageSize, struct sockaddr_in *_srcIP)
{
	srcIP = _srcIP;
	//b_COTParseError = false;
	printf("========================================\r\nCOT Port: Bytes Received: %d\r\n", messageSize);
	uint8_t* message = (uint8_t*)_message;

	++COTMsgCount;

	//HexDump(messageSize, (uint8_t*)_message);

	//printf("%02X %02X %02X\r\n", (uint8_t)_message[0], (uint8_t)_message[1], (uint8_t)_message[2]);

	if ((0xbf == (uint8_t)_message[0]) && (0x01 == (uint8_t)_message[1]) && (0xbf == (uint8_t)_message[2]))
	{
		putsRed("Found the three Protobuf magic bytes");
		ParseProtobuf((uint8_t*)&_message[3], messageSize - 3);
		ProtobufCount++;

	}

	else if (0==strncmp((const char*)message, "<?xml", 5))
	{
		ParseXMLCoT(_message, messageSize);
		XMLCount++;
	}

	else parseErrorCount++;
	//else cover XML types

}



void callbackGeoChatListener(char* _message, int messageSize, struct sockaddr_in* _srcIP)
{
	srcIP = _srcIP;
	//b_COTParseError = false;
	printf("========================================\r\nGeoChat Port: Bytes Received: %d\r\n", messageSize);
	uint8_t* message = (uint8_t*)_message;

	++GeoChatMsgCount;

	//HexDump(messageSize, (uint8_t*)_message);

	//printf("%02X %02X %02X\r\n", (uint8_t)_message[0], (uint8_t)_message[1], (uint8_t)_message[2]);

	if ((0xbf == (uint8_t)_message[0]) && (0x01 == (uint8_t)_message[1]) && (0xbf == (uint8_t)_message[2]))
	{
		putsRed("Found the three Protobuf magic bytes");
		ParseProtobuf((uint8_t*)&_message[3], messageSize - 3);
		ProtobufCount++;

	}

	else if (0 == strncmp((const char*)message, "<?xml", 5))
	{
		ParseXMLCoT(_message, messageSize);
		XMLCount++;
	}

	else parseErrorCount++;
	//else cover XML types

}



void ShutDownCOTListener()
{
	isCOTListenerRunning = false;

	if (socketCOTListen == NULL)
	{
		return;
	}
	delete socketCOTListen;
	socketCOTListen = NULL;
}


void ShutDownGEOCHATListener()
{
	isGEOCHATListenerRunning = false;

	if (socketGeoChatListen == NULL)
	{
		return;
	}
	delete socketGeoChatListen;
	socketGeoChatListen = NULL;
}

void StartCOTListener()
{
	if (TRUE == isCOTListenerRunning)
	{
		putsYellow("COT Listener already listening");
		return;//already running
	}
	if (1 == InitMcastListenerCOT(COT_MULTICAST_LISTEN_GROUP, COT_MULTICAST_LISTEN_PORT))
	{
		return;
	}
	putsGreen("Started COT Listener");
}


void StopCOTListener()
{
	ShutDownCOTListener();
	putsRed("Stopped COT Listener");
}



void StartGeoChatListener()
{
	if (TRUE == isGEOCHATListenerRunning)
	{
		putsYellow("GeoChat Listener already listening");
		return;//already running
	}
	if (1 == InitMcastListenerGeoChat(GEOCHAT_MULTICAST_LISTEN_GROUP, GEOCHAT_MULTICAST_LISTEN_PORT))
	{
		return;
	}
	putsGreen("Started GeoChat Listener");
}


void StopGEOCHATListener()
{
	ShutDownGEOCHATListener();
	putsRed("Stopped Geo Chat Listener");
}




void ShowCOTListenControls()
{
	ImGui::SeparatorText("COT Receive Control");
	ImGui::Text("%s:%d", COT_MULTICAST_LISTEN_GROUP, COT_MULTICAST_LISTEN_PORT);

	ImGuiInputTextFlags f = ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank;
	ImGui::PushItemWidth(ImGui::GetFontSize()*10);
	ImGui::InputText("COT IP: ", COT_MULTICAST_LISTEN_GROUP, 24, f);
	ImGui::InputInt("COT UDP Port", &COT_MULTICAST_LISTEN_PORT);


	if (isCOTListenerRunning)
	{
		if (RedButton("Stop COT Listener")) StopCOTListener();
	}
	else //not running
	{
		if (GreenButton("Start COT Listener")) StartCOTListener();
	}
	ImGui::PopItemWidth();
}



void ShowGeoChatListenControls()
{
	ImGui::SeparatorText("GeoChat Receive Control");
	ImGui::Text("%s:%d", GEOCHAT_MULTICAST_LISTEN_GROUP, GEOCHAT_MULTICAST_LISTEN_PORT);

	ImGuiInputTextFlags f = ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank;
	ImGui::PushItemWidth(ImGui::GetFontSize() * 10);
	ImGui::InputText("GeoChat IP: ", GEOCHAT_MULTICAST_LISTEN_GROUP, 24, f);
	ImGui::InputInt("GeoChat UDP Port", &GEOCHAT_MULTICAST_LISTEN_PORT);


	if (isGEOCHATListenerRunning)
	{
		if (RedButton("Stop GEOCHAT Listener")) StopGEOCHATListener();
	}
	else //not running
	{
		if (GreenButton("Start GEOCHAT Listener")) StartGeoChatListener();
	}
	ImGui::PopItemWidth();
}





void ShowCOTEntityListDialog(bool *pOpen)
{
	ImGui::Begin("COT Entity List", pOpen);//, ImGuiWindowFlags_AlwaysAutoResize);
	 
	if (ImGui::Button("Clear"))
	{
		//COTPLIList.clear();
	}
	
	

	ImGui::End();
}



#pragma endregion COTListener



void COTEntityLog(bool* pOpen)
{
	ImGui::Begin("COT Message Log", pOpen);

	if (ImGui::CollapsingHeader("Msg Counts", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("CoT Msgs:  %03d", COTMsgCount); ImGui::SameLine();
		ImGui::Text("Protobuf: %03d  XML: %03d", ProtobufCount, XMLCount);
		ImGui::Text("Parse Errors: %3d", parseErrorCount);
	}
	if (ImGui::Button("Clear"))
	{
		CotEntityList.clear();
		COTMsgCount = ProtobufCount = XMLCount = parseErrorCount = 0;
	}

	const int numCols = 14;

	static ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
	ImGui::BeginTable("SDW_Groups", numCols, flags);
	{
		ImGui::TableSetupColumn("callSign", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("age [s]", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("count", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableSetupColumn("Src IP", ImGuiTableColumnFlags_WidthStretch); 
		ImGui::TableSetupColumn("Format", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableSetupColumn("type", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("uid", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Lat [d.d]", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Lng [d.d]", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("hae [m]", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("ce [m]", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("le [m]", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("how", ImGuiTableColumnFlags_WidthStretch); 
		ImGui::TableSetupColumn("sendTime", ImGuiTableColumnFlags_WidthStretch);


		const char* tooltip[numCols] = {"CotEvent/Detail/Contact/Callsign","time since last Rx","", ""  ,"Protbuf or XML","Hierarchically organized hint about event type" ,"Globally unique name for this information" ,
			"CotEvent/lat referred to the WGS 84 ellipsoid in degrees" 
			,"CotEvent/lon referred to the WGS 84 ellipsoid in degrees" ,
			"Height Above WGS 84 Ellipsoid in m" ,
			"Circular 1-sigma or decimal a circular area about the point in meters" ,
			"Linear 1-sigma error or decimal an altitude range about the point in meters" ,
			"Gives a hint about how the coordinates were generated" ,"when this event was generated" };
		
		for (int column_n = 0; column_n < numCols; column_n++)
		{
			ImGui::TableNextColumn(); // Advance to the next column
			ImGui::TableHeader(ImGui::TableGetColumnName(column_n)); // Create the header

			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", tooltip[column_n]);//ImGui::TableGetColumnName(column_n));
			}
		}


		//ImGui::TableHeadersRow();

		for (CotEvent* cev : CotEntityList)
		{

			if (cev->age> K5_1_StaleTime) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			else ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
			int y = 0;
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(y++);
			if (nullptr != cev->detail->contact)
				ImGui::Text("%s", cev->detail->contact->callsign.c_str());
			else if (cev->detail->st_xmlDetail->x_callSign.size() != 0)
			{
				ImGui::TextColored(ImColor(255, 0, 0, 255), "%s", cev->detail->st_xmlDetail->x_callSign.c_str());
			}


			
			ImGui::TableSetColumnIndex(y++);
			ImGui::Text("%d", cev->age);

			ImGui::TableSetColumnIndex(y++);
			ImGui::Text("%d", cev->count);

			ImGui::TableSetColumnIndex(y++);
			ImGui::Text("%d:%d:%d:%d", cev->srcIP[0], cev->srcIP[1], cev->srcIP[2], cev->srcIP[3]);

			ImGui::TableSetColumnIndex(y++);
			if (CotEvent::MsgFormat::PROTOBUF == cev->msgFormat) ImGui::Text("Pbuf");
			else if (CotEvent::MsgFormat::XML_EVENT == cev->msgFormat) ImGui::Text("XML/Event");
			else if (CotEvent::MsgFormat::XML_MSG == cev->msgFormat) ImGui::Text("XML/Msg");
			

			ImGui::TableSetColumnIndex(y++);
			ImGui::Text("%s", cev->type.c_str());
			

			ImGui::TableSetColumnIndex(y++);
			ImGui::Text("%s", cev->uid.c_str());
			ImGui::TableSetColumnIndex(y++);
			ImGui::Text("%7.4f", cev->lat);
			
			ImGui::TableSetColumnIndex(y++);
			ImGui::Text("%8.4f", cev->lon);

			ImGui::TableSetColumnIndex(y++);
			ImGui::Text("%3.2f", cev->hae);
			

			ImGui::TableSetColumnIndex(y++);
			ImGui::Text("%3.2f", cev->ce);

			ImGui::TableSetColumnIndex(y++);
			ImGui::Text("%3.2f", cev->le);

			ImGui::TableSetColumnIndex(y++);
			ImGui::Text("%s", cev->how.c_str());

			ImGui::TableSetColumnIndex(y++);
			ImGui::Text("%s", cev->s_sendTime.c_str());
			
			ImGui::PopStyleColor();

		}
	}
	ImGui::EndTable();



	ImGui::End();

}






#pragma region COTSimTimer

void StartCOTTimer()
{
	int SpeedUpFactor = 1;
	int sendPeriod = 1;  //this will be 1000ms = 1sec without speedup (SpeedUpFactor==1)
	CreateTimerQueueTimer(&ptrTimerHandle, NULL, CoTTimerCallback, NULL, 500, sendPeriod * 1000 / SpeedUpFactor, WT_EXECUTEDEFAULT); //send ASX every sendPeriod seconds
	puts("Started COT Timer");
}


void StopCOTTimer()
{
	bool waste = DeleteTimerQueueTimer(NULL, ptrTimerHandle, NULL);
	puts("Stopped COT Timer");
}


void UpdateAllCOTPositions()
{
	putsRed("CoT Timer UpdateAllCOTPositions");

	/*
	std::lock_guard<std::mutex> lock(VMFPAList_mtx);
	for (auto v : K5_1PAList)
	{
		if (!v->isSim) continue;
		v->EntityLat += .0001;
		v->EntityLng += .0001;
		v->age = 0;
	}
	*/
}


static void __stdcall CoTTimerCallback(PVOID, BOOLEAN)//(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	if (isCOTSimRunning) UpdateAllCOTPositions();
	
	for (auto v : CotEntityList)
	{
		static int updateCount = -1;  //-1 forces first pass to send mesaage
		v->age++;
		//if (++updateCount % 5) return;
	}
	
}


#pragma endregion COTSimTimer


void COTChatLog(bool* pOpen)
{
	ImGui::Begin("CoT Chat Log", pOpen);

	ImGui::SeparatorText("Msg Counts");
	//ImGui::Text("K1.1: %03d", K1_1Count);
	ImGui::Separator();

	for (auto m : CotChatList)
	{
		ImGui::Text("%s \t%s \t%s", m->detail->st_xmlDetail->chatRoom.c_str(), m->detail->st_xmlDetail->senderCallSign.c_str(), m->detail->st_xmlDetail->remarks_text.c_str());
	}

	ImGui::End();
}


/////////////


void COTModule::ShowCOTControlDialog(bool* pOpen)
{
	ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
	ImGui::Begin("COT Control", pOpen, flags);

	ShowCOTListenControls();
	ShowCOTSendControls();
	ShowGeoChatListenControls();

	ImGui::End();
}



#include "AppIni.h"
void COTModule::Initialize(ImVec2(*funcLatLon2VPxy)(double, double))
{
	FuncPtrLLtoVPxy = funcLatLon2VPxy;  //not used without map module

	LoadIniValues();
	CoTSenderLoadIniValues();

	StartCOTTimer();

	
	printf("COT Initialized\r\n");
}

void COTModule::ShutDown()
{
	SaveIniValues();
	StopCOTTimer();
}


void COTModule::LoadIniValues()
{
	puts("Loading COTModule ini values");

	b_ShowDialog = GetIniBool("mb_ShowCOTList", false);
	mb_ShowCOTChatLog = GetIniBool("mb_ShowCOTChatLog", false);

	mb_ShowCOTMessageLog = GetIniBool("mb_ShowCOTMessageLog", false);

	b_ShowLatestCOTMessage = GetIniBool("b_ShowLatestCOTMessage", true);
	mb_ShowCOTPLIEntityList = GetIniBool("mb_ShowCOTPLIEntityList", true);

	mb_ShowCOTMsgBuilder = GetIniBool("mb_ShowCOTMsgBuilder", true);




	COT_MULTICAST_LISTEN_PORT = GetIniInt("COT_MULTICAST_LISTEN_PORT", 6969);
	std::string s = GetIniString("COT_MULTICAST_LISTEN_GROUP", "239.2.3.1");
	strncpy(COT_MULTICAST_LISTEN_GROUP, s.c_str(), 20);
	bool StartVMFListenerOnBoot = GetIniBool("StartCOTListenerOnBoot", false);
	if (StartVMFListenerOnBoot) StartCOTListener();

	COT_MULTICAST_SEND_PORT = GetIniInt("COT_MULTICAST_SEND_PORT", 6969);
	s = GetIniString("COT_MULTICAST_SEND_GROUP", "239.2.3.1");
	strncpy(COT_MULTICAST_SEND_GROUP, s.c_str(), 20);
	bool StartCOTSenderOnBoot = GetIniBool("StartCOTSenderOnBoot", false);
	if (StartCOTSenderOnBoot) StartCOTSender();


	GEOCHAT_MULTICAST_LISTEN_PORT = GetIniInt("GEOCHAT_MULTICAST_LISTEN_PORT", 17012);
	s = GetIniString("GEOCHAT_MULTICAST_LISTEN_GROUP", "224.10.10.1");
	strncpy(GEOCHAT_MULTICAST_LISTEN_GROUP, s.c_str(), 20);
	bool StartGEOCHATListenerOnBoot = GetIniBool("StartGEOCHATListenerOnBoot", false);
	if (StartGEOCHATListenerOnBoot) StartGeoChatListener();

};


void COTModule::SaveIniValues()
{
	puts("Saving COTModule ini values");
	UpdateIniBool("mb_ShowCOTList", b_ShowDialog);
	UpdateIniBool("mb_ShowCOTChatLog", mb_ShowCOTChatLog);
	UpdateIniBool("mb_ShowCOTMessageLog", mb_ShowCOTMessageLog);
	

	UpdateIniBool("b_ShowLatestCOTMessage", b_ShowLatestCOTMessage);
	UpdateIniBool("mb_ShowCOTPLIEntityList", mb_ShowCOTPLIEntityList);
	UpdateIniBool("mb_ShowCOTMsgBuilder", mb_ShowCOTMsgBuilder);

	//UpdateIniBool("mb_ShowEntityList", mb_ShowEntityList);
	//UpdateIniBool("mb_ShowCOTMesageLog", mb_ShowCOTMesageLog);
	//UpdateIniBool("mb_ShowCOTMsgBuilder", mb_ShowCOTMsgBuilder);
	

	UpdateIniInt("COT_MULTICAST_LISTEN_PORT", COT_MULTICAST_LISTEN_PORT);
	UpdateIniString("COT_MULTICAST_LISTEN_GROUP", COT_MULTICAST_LISTEN_GROUP);
	if (isCOTListenerRunning) UpdateIniBool("StartCOTListenerOnBoot", true);
	else UpdateIniBool("StartCOTListenerOnBoot", false);

	UpdateIniInt("COT_MULTICAST_SEND_PORT", COT_MULTICAST_SEND_PORT);
	UpdateIniString("COT_MULTICAST_SEND_GROUP", COT_MULTICAST_SEND_GROUP);
	if (isCOTSenderRunning) UpdateIniBool("StartCOTSenderOnBoot", true);
	else UpdateIniBool("StartCOTSenderOnBoot", false);


	UpdateIniInt("CGEOCHAT_MULTICAST_LISTEN_PORT", GEOCHAT_MULTICAST_LISTEN_PORT);
	UpdateIniString("GEOCHAT_MULTICAST_LISTEN_GROUP", GEOCHAT_MULTICAST_LISTEN_GROUP);
	if (isGEOCHATListenerRunning) UpdateIniBool("StartGEOCHATListenerOnBoot", true);
	else UpdateIniBool("StartGEOCHATListenerOnBoot", false);



};


void COTModule::Menu()
{
	if (ImGui::BeginMenu("COT"))
	{
		if (ImGui::MenuItem("COT Control", NULL, b_ShowDialog)) { b_ShowDialog = !b_ShowDialog; }
		if (ImGui::MenuItem("Entity List", NULL, mb_ShowEntityList)) { mb_ShowEntityList = !mb_ShowEntityList; }
		if (ImGui::MenuItem("COT Msg Log", NULL, mb_ShowCOTMessageLog)) { mb_ShowCOTMessageLog = !mb_ShowCOTMessageLog; }
		if (ImGui::MenuItem("COT Msg Builder", NULL, mb_ShowCOTMsgBuilder)) { mb_ShowCOTMsgBuilder = !mb_ShowCOTMsgBuilder; }
		if (ImGui::MenuItem("COT Chat Log", NULL, mb_ShowCOTChatLog)) { mb_ShowCOTChatLog = !mb_ShowCOTChatLog; }
		ImGui::Separator();

		if (!isCOTSimRunning)
		{
			if (ImGui::MenuItem("Start COT Sim", NULL)) { isCOTSimRunning=true; }
		}
		else 
			if (ImGui::MenuItem("Stop COT Sim", NULL)) { isCOTSimRunning = false; }


		ImGui::EndMenu();
	}
}



void COTModule::ShowDialogs()
{
	if (b_ShowDialog) ShowCOTControlDialog(&b_ShowDialog);
	if (mb_ShowCOTChatLog) COTChatLog(&mb_ShowCOTChatLog);
	if (mb_ShowCOTMsgBuilder) ShowCOTMsgBuilderDialog(&mb_ShowCOTMsgBuilder);
	if (mb_ShowCOTMessageLog) CoTMsgLog(&mb_ShowCOTMessageLog);
	if (mb_ShowCOTPLIEntityList) COTEntityLog(&mb_ShowCOTPLIEntityList);
	
	/*
	if (mb_ShowEntityList) ShowCOTEntityListDialog(&mb_ShowEntityList);

	if (mb_ShowCOTMesageLog) MsgLog(&mb_ShowCOTMesageLog);

	
	if (mb_ShowCOTChatLog) ChatLog(&mb_ShowCOTChatLog);
	*/

}


static void SetCoTLogFilters(bool val)
{
	filter_Type_a = val;
	filter_Type_t = val;
	filter_Type_x = val;

}


void CoTMsgLog(bool* pOpen)
{
	static ImVec2 lastWinPos;
	/*
	if (trap)
	{
		if ((lastWinPos.x> g_displaySize.x) || (lastWinPos.y > g_displaySize.y))
			ImGui::SetNextWindowPos(ImVec2(100, 100));
	}
	*/

	ImGui::Begin("CoT PLI Message Log", pOpen);

	lastWinPos = ImGui::GetWindowPos();
	//ImGui::Text("Display Size: %f %f, CurWinPos: %f %f", g_displaySize.x, g_displaySize.y, lastWinPos.x, lastWinPos.y);



	//ImGui::SeparatorText("Msg Counts");

	if (ImGui::CollapsingHeader("Msg Counts"))
	{
		
	}

	CotEvent* p = nullptr;
	if (ImGui::CollapsingHeader("Msg Log", ImGuiTreeNodeFlags_DefaultOpen))
	{
		char buf[40];
		static int numToShow = CotEventList.size();  //index from 1 thru size (not 0 through size -1)

		ImGui::Checkbox("Show Latest", &b_ShowLatestCOTMessage);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6);
		sprintf(buf, "Message # of %d", (int)(CotEventList.size()));
		if (ImGui::InputInt(buf, &numToShow)) b_ShowLatestCOTMessage = false;


		if (0 == CotEventList.size())
		{
			numToShow = 1;
		}
		else
		{
			if (b_ShowLatestCOTMessage) numToShow = CotEventList.size();
			else numToShow = std::clamp(numToShow, 1, (int)CotEventList.size());
		}


		/*
		//filtering
		if (ImGui::Button("Clear Filters")) SetCoTLogFilters(false); ImGui::SameLine();
		if (ImGui::Button("Set All Filters")) SetCoTLogFilters(true);

		ImGui::Text("FIMs"); ImGui::SameLine(ImGui::GetFontSize() * 3);
		ImGui::Checkbox("01", &filter_Type_a);ImGui::SetItemTooltip("a - atomsd");ImGui::SameLine();
		ImGui::Checkbox("02", &filter_Type_t);ImGui::SetItemTooltip("t - task");ImGui::SameLine();
		ImGui::Checkbox("03", &filter_Type_x);ImGui::SetItemTooltip("x - ?");ImGui::SameLine();


		std::vector<CotEvent*> xref;
		for (auto p : CotEventList)
		{
			if ( ('x' == p->type[0]) && filter_Type_a) xref.push_back(p);
			if ( ('t'  == p->type[0]) && filter_Type_t) xref.push_back(p);
			if ( ('x'  == p->type[0]) && filter_Type_x) xref.push_back(p);

		}

		ImGui::Text("Num Filtered Msgs: %d", xref.size());

		static int numToShow = xref.size();  //index from 1 thru size (not 0 through size -1)
		ImGui::Checkbox("Show Latest", &b_ShowLatestCOTMessage);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7);
		sprintf(buf, "Message # of %d", (int)(xref.size()));
		if (ImGui::InputInt(buf, &numToShow)) b_ShowLatestCOTMessage = false;

		numToShow = std::clamp(numToShow, 0, (int)CotEventList.size());
		if (0 == xref.size())
		{
			numToShow = 1;
		}
		else
		{
			if (b_ShowLatestCOTMessage) numToShow = xref.size();
			else numToShow = std::clamp(numToShow, 1, (int)xref.size());
		}

		*/


		//ImGui::Checkbox("Log K5.1", &b_LogK5_1);
		ImGui::SameLine();
		if (ImGui::Button("Clear Log"))
		{
			std::lock_guard<std::mutex> lock(COTMsgList_mtx);
			CotEventList.clear();
		}


		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
		//if (ImGui::BeginChild("ResizableChild", ImVec2(-FLT_MIN, ImGui::GetTextLineHeightWithSpacing() * 40), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY))
		
		if (ImGui::BeginChild("ResizableChildCoT", ImVec2(-FLT_MIN, -FLT_MIN), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY))

		std::lock_guard<std::mutex> lock(COTMsgList_mtx);
		if (CotEventList.size() > 0)
		{
			p = CotEventList[numToShow - 1];
			//ImGui::Text("Msg Size: %d bits -> %d bytes", p->msgSizeInBits, p->msgSizeInBytes);

			p->pme();
		}


		ImGui::PopStyleColor();

		ImGui::EndChild();
	}
	ImGui::End();

		

}



