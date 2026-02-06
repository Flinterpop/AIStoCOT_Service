#pragma once

#include <vector>

//AIS
/*
* from pyatk
DEFAULT_COT_TYPE: str = "a-u-S-X-M"




from constants.py (AISCOT)

DEFAULT_COT_URL: str = "udp+wo://239.2.3.1:6969"  # ATAK Default multicast
DEFAULT_COT_STALE: str = "120"  # Config wants all values as strings, we'll cast later.
DEFAULT_HOST_ID: str = f"pytak@{platform.node()}"
DEFAULT_COT_PORT: str = "8087"
DEFAULT_ATAK_PORT: str = "4242"
DEFAULT_BROADCAST_PORT: str = "6969"

DEFAULT_BACKOFF: str = "120"
DEFAULT_SLEEP: str = "5"


# Python <3.8 has no way of including XML Declaration in ET.tostring():
DEFAULT_XML_DECLARATION: bytes = (
	b'<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>'
)


# See MIL-STD-6090.
DEFAULT_COT_ACCESS: Optional[str] = os.getenv("COT_ACCESS", "UNCLASSIFIED")
DEFAULT_COT_CAVEAT: Optional[str] = os.getenv("COT_CAVEAT", "")
DEFAULT_COT_RELTO: Optional[str] = os.getenv("COT_RELTO", "")
DEFAULT_COT_QOS: Optional[str] = os.getenv("COT_QOS", "")
DEFAULT_COT_OPEX: Optional[str] = os.getenv("COT_OPEX", "")
*/


inline void encodeVarint(uint64_t value, std::vector<uint8_t>& output) {
	while (true) {
		if ((value & ~0x7FULL) == 0) {
			output.push_back(static_cast<uint8_t>(value));
			return;
		}
		else {
			output.push_back(static_cast<uint8_t>((value & 0x7FULL) | 0x80ULL));
			value >>= 7;
		}
	}
}



struct bg_TakMessage
{

public:
	int tm_validTimeInSeconds = 90;

	void AssembleCoTPbufEvent();
	int build_CoTEvent();
	int buildCoTEvent_Detail();
	int build_Detail_XmlDetail();
	int build_Detail_Contact();
	int build_Detail_Group();
	int build_Detail_precisionLocation();
	int build_Detail_status();
	int build_Detail_takv();	
	int build_Detail_Track();



protected:

	void _insertBuf(std::vector<unsigned char>& v, unsigned char b[], int l)
	{
		for (int x = 0; x < l; x++) v.push_back(b[x]);
	};

	void insertString(std::vector<unsigned char>& v, const char* b)
	{
		int l = strlen(b);
		encodeVarint(l, v);
		for (int x = 0; x < (strlen(b)); x++) v.push_back(b[x]);
	};

	void insertDouble(std::vector<unsigned char>& v, double d)
	{
		uint64_t i64rep;
		memcpy(&i64rep, &d, sizeof(double));
		unsigned char* p = (unsigned char*)&i64rep;
		for (int i = 0; i < 8; i++) v.push_back(p[i]);
	};

	void insertInt(std::vector<unsigned char>& v, int d)
	{
		//uint64_t i64rep;
		//memcpy(&i64rep, &d, sizeof(double));
		//unsigned char* p = (unsigned char*)&i64rep;
		//for (int i = 0; i < 8; i++) v.push_back(p[i]);
		v.push_back(d);
	};




protected:
	std::vector<unsigned char> CoTEvent;  //uint8_t
	std::vector<unsigned char> CoTDetail;
	std::vector<unsigned char> CoTXmlDetail;
	std::vector<unsigned char> CoTGroup;
	std::vector<unsigned char> CoTContact;
	std::vector<unsigned char> CoTTrack;
	std::vector<unsigned char> CoTStatus;
	std::vector<unsigned char> CoTTakv;
	std::vector<unsigned char> CoTPrecisionLocation;
public:
	std::vector<unsigned char> PKT;

	bool IncludeTakControl = false;
	bool includeContact = false;
	bool includeGroup = false;
	
	//CoTEvent
	//char msg_type[30] = { "a-f-G" };
	std::string msg_type{};
	//char msg_type[30] = { "a-f-G-I-U-T" };  //CRS Vessel
	//char msg_type[30] = { "a-f-S-X-M" }; 
	//char msg_type[30] = { "a-f-S-X-L" }; //SAR Vesel
	
	std::string access{};//optional
	std::string qos{};//optional
	std::string opex{};//optional
	std::string UID{};

	
	int COT_STALE{};//valid time in seconds

	std::string _how{ "h-e"};

	double d_lat = 45.5001;
	double d_lon = -75.0505;
	double d_hae = 101;
	double d_ce = 999999;
	double d_le = 999999;


	//CoT Detail //optional
	bool includeDetail = false;
	std::string xmlDetail{}; //optional
	
	//Detail - Contact
	std::string endpoint{};// "192.168.19.119:4242:tcp" }; //optional
	std::string callsign{};// "wxAIS"};
	
	//Detail - Group
	std::string group_name{};// "Yellow"};
	std::string role{};// "Team Member"};


	//Detail - Precision Location
	std::string geopointsrc{};
	std::string altsrc{};
	 
	//Detail - status
	int battery{ -1 };

	//Detail - Takv
	std::string device{};
	std::string platform{};
	std::string os{};
	std::string version{};

	//Detail - Track
	double course = -99;
	double speed = -99;


	//link

	//model


	//remarks

	//archive


	



};

