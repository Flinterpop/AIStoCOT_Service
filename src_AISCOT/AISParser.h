#pragma once

#include <sstream>
#include <format>
#include <ranges>
#include <algorithm>

#include "ais.h"
#include "decode_body.h"


using namespace libais;

extern const char* NAV_STATUS[];
extern int MsgCounts[27];



/* NMEA sentence max length, including \r\n (chars) */
#define NMEA_MAX_LENGTH		82

/* NMEA sentence endings, should be \r\n according the NMEA 0183 standard */
#define NMEA_END_CHAR_1		'\r'
#define NMEA_END_CHAR_2		'\n'

/* NMEA sentence prefix length (num chars), Ex: GPGLL */
#define NMEA_PREFIX_LENGTH	5




    inline bool isStringADouble(const std::string& s) {
        char* end = nullptr;
        double val = std::strtod(s.c_str(), &end);

        // Check if a conversion was performed (end != s.c_str())
        // and if the entire string was consumed (*end == '\\0')
        // This also needs to handle potential issues like underflow/overflow if necessary
        return end != s.c_str() && *end == '\0';
    }


    inline bool isStringAnInteger(const std::string& s) {
        if (s.empty()) return false;

        char* p;
        // strtol attempts to parse the string as a long integer
        // The second argument, &p, is set to point to the character 
        // where parsing stopped. The third argument is the base (10 for decimal).
        long converted_val = std::strtol(s.c_str(), &p, 10);

        // Check if the pointer 'p' reached the end of the string.
        // Also check for leading whitespace, which strtol ignores by default
        // but may be considered invalid for a strict integer check.
        // The *p == 0 check ensures no non-integer characters (like 'a', '.') remain.
        return (*p == 0);
    }



    namespace AIS_PARSER
    {

        struct NMEA_AIS_MSG
        {
            bool isValid = false;
            std::string parseRecordString{};

            std::string sentence{};  //example !AIVDM,1,1,,A,1Cu?etPjh0J`ej@Ih@B1hQH00000,0*5B
            std::string name{};         //Field 1 should be AIVDM
            int CountOfFragments{};     //Field 2   1 or 2
            int FragmentNumber{};       //Field 3   1 or 2 
            int SequentialMessageID{};  //Field 4   often 0, shoudl be the same for fragments of the same message
            std::string RadioChannel{}; //Field 5   A or B 
            std::string payload{};      //Field 6   string of binary data 
            int fillBits{};             //Field 7 before *
            int checksum{};             //Field 7 after *


            NMEA_AIS_MSG(std::string NMEA_Sentence)
            {
                ////wxLogMessage("Parsing %s", NMEA_Sentence);

                std::stringstream retVal{};
                std::vector<std::string> fields;
                auto split_view = NMEA_Sentence | std::ranges::views::split(',');
                for (const auto& view : split_view) fields.push_back(std::string(view.begin(), view.end()));
                for (const std::string& fields : fields) std::cout << fields << std::endl;
                if (fields.size() != 7)
                {
                    parseRecordString = "Incorrect number of fields";
                    return;
                }

                retVal << "Num Fields: " << fields.size() << std::endl;
                for (auto s : fields)
                    retVal << s << "//";

                sentence = NMEA_Sentence;
                name = fields[0];

                bool isInt = isStringAnInteger(fields[1]);
                if (isInt) CountOfFragments = std::stoi(fields[1]);

                isInt = isStringAnInteger(fields[2]);
                if (isInt) FragmentNumber = std::stoi(fields[2]);

                isInt = isStringAnInteger(fields[3]);
                if (isInt) SequentialMessageID = std::stoi(fields[3]);

                RadioChannel = fields[4];
                payload = fields[5];

                char FB = fields[6][0];
                fillBits = FB - 0x30;

                std::string c = fields[6].substr(2);

                isInt = isStringAnInteger(c);
                if (isInt) checksum = std::stoi(c);

                parseRecordString = retVal.str();
                isValid = true;
            }

            std::string print()
            {
                std::stringstream retVal{};
                retVal << "AIS NMEA Sentence: " << std::endl;
                retVal << sentence << std::endl;
                retVal << std::format("name: {}\r\n", name);
                retVal << std::format("Num Frags: {}\r\n", CountOfFragments);
                retVal << std::format("Frag Num: {}\r\n", FragmentNumber);
                retVal << std::format("Msg ID: {}\r\n", SequentialMessageID);
                retVal << std::format("RadioChannel: {}\r\n", RadioChannel);
                retVal << std::format("Payload: {}\r\n", payload);
                retVal << std::format("fill bits: {}\r\n", fillBits);

                retVal << std::format("Checksum: {}\r\n", checksum);

                return retVal.str();
            }

        };

        struct KnownVessel
        {
            int MMSI{};
            int IMO{};
            std::string name{};
            std::string callsign{};
            int A{}, B{}, C{}, D{};  //dimensions
            int type{};
            std::string flag{};

            KnownVessel(int mmsi, int imo, std::string _name, std::string cs, int _type, std::string _flag, int a, int b, int c, int d)
            {
                MMSI = mmsi;
                IMO = imo;
                name = _name;
                callsign = cs;
                A = a;
                B = b;
                C = c;
                D = d;  //dimensions
                type = _type;
                flag = _flag;  //Country of registration
            };
        };



        class AISObject  //base class for all AIS things with an MMSI
        {
        public:
            int mmsi = 0;
            int age = 0;
            bool markForDelete = false;
            int AISMsgNumber = 0; //1 thru 27


        public:
            AISObject(int _AISMsgNum, int _mmsi)
            {
                mmsi = _mmsi;
                AISMsgNumber = _AISMsgNum;
                MsgCounts[AISMsgNumber]++;
            };

            virtual std::string LogMe() {
                std::stringstream retVal{};
                retVal << "AISObject (base class) " << std::endl;
                retVal << "MMSI " << mmsi << std::endl;
                retVal << "Message ID " << AISMsgNumber << std::endl;
                return retVal.str();
            };


        };

        class AidToNavigation : public AISObject
        {
            AidToNavigation(Ais6* a) : AISObject(a->message_id, a->mmsi)
            {
                asi6 = a;
            };

            Ais6* asi6{};

            std::string LogMe() override
            {
                std::stringstream retVal{};
                retVal << "AIS 6 Ait To Nav parse: " << std::endl;
                retVal << "MMSI " << mmsi << std::endl;
                return retVal.str();
            }

        };

        class Vessel : public AISObject
        {
        public:

            Vessel(Ais1_2_3* a) : AISObject(a->message_id, a->mmsi)
            {
                a123 = a;
                isValidAIS123 = true;
            };
            Vessel(Ais18* a) : AISObject(a->message_id, a->mmsi)
            {
                a18 = a;
                isValidAIS18 = true;
            };

            Vessel(Ais5* a) : AISObject(a->message_id, a->mmsi)
            {
                ais5 = a;
                isValidAIS5 = true;
            };

            Vessel(Ais24* a) : AISObject(a->message_id, a->mmsi)
            {
                ais24 = a;
                isValidAIS24 = true;
            };



            Ais1_2_3* a123{};   //Class A Position Reports
            Ais5* ais5{};       //Class A Ship Data

            Ais18* a18{};       //Class A Position Reports
            Ais24* ais24{};       //Class A Ship Data


            bool isValidAIS123{ false };
            bool isValidAIS5{ false };

            bool isValidAIS18{ false };
            bool isValidAIS24{ false };


            //AIS 1,2,3, 18

            int position_accuracy{};
            AisPoint position{};
            double lat_deg{};
            double lng_deg{};
            float cog{};  // Degrees.
            float sog{};
            int true_heading{};
            int timestamp{};
            int special_manoeuvre{};
            bool raim{};
            bool utc_valid{};
            int utc_hour{};
            int utc_min{};

            //AIS 1,2,3
            AIS_NAVIGATIONAL_STATUS nav_status{};


            //AIS 5, 24
            int ais_version{};
            int imo_num{};
            std::string callsign{};
            std::string name{};  //Vessel Names that exceed the AIS’s 20 character limit should be shortened (not truncated) to 15 character - spaces, followed by an underscore{ _ },
            int type_and_cargo{};
            int dim_a{};
            int dim_b{};
            int dim_c{};
            int dim_d{};
            int fix_type{};

            int eta_month{};
            int eta_day{};
            int eta_hour{};
            int eta_minute{};
            float draught{};  // present static draft. m
            std::string destination{};
            int dte{};

            //AIS 24
            int Mothership_MMSI{};

            std::string LogMe() override
            {
                std::stringstream retVal{};
                retVal << "AIS_1_2_3 parse: " << std::endl;
                retVal << "MMSI " << mmsi << std::endl;

                if (nullptr != ais5)
                {
                    retVal << "callsign " << ais5->callsign << std::endl;
                    retVal << "name " << ais5->name << std::endl;
                    retVal << "type_and_cargo " << ais5->type_and_cargo << std::endl;
                    retVal << "destination " << ais5->destination << std::endl;
                }

                if (nullptr != a123)
                {

                    retVal << "nav_status " << NAV_STATUS[nav_status] << std::endl;
                    retVal << "true_heading " << true_heading << std::endl;
                    retVal << "position, lat " << position.lat_deg << std::endl;
                    retVal << "position, lng " << position.lng_deg << std::endl;
                    retVal << "time stamp " << timestamp << std::endl;
                }

                return retVal.str();


            }
        };



        class VesselClassA : public Vessel
        {

        };

        class VesselClassB : public Vessel
        {

        };



        void BuildKnownVesselList();
        KnownVessel* FindKnownVesselByMMSI(int mmsi);
        Vessel* FindVesselByMMSI(int mmsi);
        AISObject* ParsePayloadString(std::string body);
        AISObject* ParseAIS123_PosReportPayload(std::string body, int fillbits);
        AISObject* ParseAIS18_PosReportPayload(std::string body, int fillbits);
        AISObject* ParseASI5IdentPayload(std::string body, int fillbits);
        AISObject* ParseASI24IdentPayload(std::string body, int fillbits);


    }