
#include "AISParser.h"

//#include <wx/log.h>

int MsgCounts[27]{};



std::vector<AIS_PARSER::Vessel*> VesselList;

std::vector<AIS_PARSER::KnownVessel*> KnownVesselList;

const char* NAV_STATUS[] = { "AIS_NV_STATUS_UNDER_WAY_USING_ENGINE",
    "AIS_NV_STATUS_AT_ANCHOR",
    "AIS_NV_STATUS_NOT_UNDER_COMMAND",
    "AIS_NV_STATUS_RESTRICTED_MANEUVERABILITY",
    "AIS_NV_STATUS_CONSTRAINED_BY_DRAUGHT",
    "AIS_NV_STATUS_MOORED",
    "AIS_NV_STATUS_AGROUND",
    "AIS_NV_STATUS_ENGAGED_IN_FISHING",
    "AIS_NV_STATUS_UNDER_WAY_SAILING",
    "AIS_NV_STATUS_RESERVED1", // reserved for future amendment of navigational status for ships carrying DG, HS, or MP, or IMO hazard or pollutant category C, high-speed craft (HSC)
    "AIS_NV_STATUS_RESERVED2", // reserved for future amendment of navigational status for ships carrying dangerous goods (DG), harmful substances (HS) or marine pollutants (MP), or IMO hazard or pollutant category A, wing in ground (WIG)
    "AIS_NV_STATUS_TOWING_ASTERN", // power-driven vessel towing astern (regional use)
    "AIS_NV_STATUS_PUSHING_AHEAD_OR_TOWING_ALONGSIDE", // power-driven vessel pushing ahead or towing alongside (regional use)
    "AIS_NV_STATUS_RESERVED3", // reserved for future use
    "AIS_NV_STATUS_SART", // AIS-SART (active), MOB-AIS, EPIRB-AIS
    "   ", // undefined = default (also used by AIS-SART, MOB-AIS and EPIRB-AIS under test)
};




void AIS_PARSER::BuildKnownVesselList()
{
    AIS_PARSER::KnownVessel* kv = new AIS_PARSER::KnownVessel(316130000, 0, "Charlettetown", "CGAJ", 35, "Canada", 134, 17, 0, 0);
    KnownVesselList.push_back(kv);

    kv = new AIS_PARSER::KnownVessel(316138000, 0, "Halifax", "CGAP", 35, "Canada", 134, 17, 0, 0);
    KnownVesselList.push_back(kv);

    kv = new AIS_PARSER::KnownVessel(316135000, 0, "Toronto", "CGAD", 35, "Canada", 134, 17, 0, 0);
    KnownVesselList.push_back(kv);

    kv = new AIS_PARSER::KnownVessel(316030879, 9348182, "Asterix", "CFN7327", 35, "Canada", 183, 34, 0, 0);
    KnownVesselList.push_back(kv);
}

AIS_PARSER::KnownVessel* AIS_PARSER::FindKnownVesselByMMSI(int mmsi)
{
    for (auto kv : KnownVesselList)
    {
        if (kv->MMSI == mmsi) return kv;
    }
    return nullptr;
}


AIS_PARSER::Vessel* AIS_PARSER::FindVesselByMMSI(int mmsi)
{
    for (auto v : VesselList)
    {
        if (v->mmsi == mmsi) return v;
    }
    return nullptr;
}




AIS_PARSER::AISObject * AIS_PARSER::ParsePayloadString(std::string body)
{
    switch (body[0])
    {
    case '1':  // FALLTHROUGH
    case '2':  // FALLTHROUGH
    case '3':  // 1-3: Class A position report.
    {
        return AIS_PARSER::ParseAIS123_PosReportPayload(body, 0);
        break;
    }


    case '4':  // FALLTHROUGH - 4 - Basestation report
    case ';':  // 11 - UTC date response
    {
        //return MakeUnique<libais::Ais4_11>(body.c_str(), fill_bits);
        break;
    }

    case '5':  // 5 - Ship and Cargo
    {
        return AIS_PARSER::ParseASI5IdentPayload(body, 2);
        //return MakeUnique<libais::Ais5>(body.c_str(), fill_bits);
        break;
    }

    case '6':  // 6 - Addressed binary message
    {
        //return CreateAisMsg6(body, fill_bits);
        break;
    }

    case '7':  // FALLTHROUGH - 7 - ACK for addressed binary message
    case '=':  // 13 - ASRM Ack  (safety message)
    {
        //   return MakeUnique<libais::Ais7_13>(body.c_str(), fill_bits);
        break;
    }

    case '8':  // 8 - Binary broadcast message (BBM)
    {
        //return CreateAisMsg8(body, fill_bits);
        break;
    }


    case '9':  // 9 - SAR Position
    {
        //return MakeUnique<libais::Ais9>(body.c_str(), fill_bits);
        break;
    }


    case ':':  //  10 - UTC Query
    {
        //return MakeUnique<libais::Ais10>(body.c_str(), fill_bits);
        break;
    }
    // ';' 11 - See 4

/*
case '<':  // 12 - Addressed Safety Related Messages (ASRM)
    return MakeUnique<libais::Ais12>(body.c_str(), fill_bits);

    // '=' 13 - See 7

case '>':  // 14 - Safety Related Broadcast Message (SRBM)
    return MakeUnique<libais::Ais14>(body.c_str(), fill_bits);

case '?':  // 15 - Interrogation
    return MakeUnique<libais::Ais15>(body.c_str(), fill_bits);

case '@':  // 16 - Assigned mode command
    return MakeUnique<libais::Ais16>(body.c_str(), fill_bits);

case 'A':  // 17 - GNSS broadcast
    return MakeUnique<libais::Ais17>(body.c_str(), fill_bits);
    */

    case 'B':  // 18 - Position, Class B
    {
        return AIS_PARSER::ParseAIS18_PosReportPayload(body, 0);
        //return MakeUnique<libais::Ais18>(body.c_str(), fill_bits);
        break;
    }


    case 'C':  // 19 - Position and ship, Class B
    {
        //return MakeUnique<libais::Ais19>(body.c_str(), fill_bits);
        break;
    }

    case 'H':  // 24 - Static data report
        return AIS_PARSER::ParseASI24IdentPayload(body, 0);   //MakeUnique<libais::Ais24>(body.c_str(), fill_bits);
        break;




    /*
    case 'D':  // 20 - Data link management
        return MakeUnique<libais::Ais20>(body.c_str(), fill_bits);

    case 'E':  // 21 - Aids to navigation report
        return MakeUnique<libais::Ais21>(body.c_str(), fill_bits);

    case 'F':  // 22 - Channel Management
        return MakeUnique<libais::Ais22>(body.c_str(), fill_bits);

    case 'G':  // 23 - Group Assignment Command
        return MakeUnique<libais::Ais23>(body.c_str(), fill_bits);


    case 'I':  // 25 - Single slot binary message
        return MakeUnique<libais::Ais25>(body.c_str(), fill_bits);

    case 'J':  // 26 - Multi slot binary message with comm state
        return MakeUnique<libais::Ais26>(body.c_str(), fill_bits);

    case 'K':  // 27 - Long-range AIS broadcast message
        return MakeUnique<libais::Ais27>(body.c_str(), fill_bits);

*/

    }
    return nullptr;

}




AIS_PARSER::AISObject * AIS_PARSER::ParseASI5IdentPayload(std::string body, int fillbits)
{
    std::unique_ptr<libais::AisMsg>  p = CreateAisMsg(body, 0);
    if (nullptr == p)
    {
        std::cout << "Null ptr" << std::endl;
        return nullptr;
    }
    else
    {
        Ais5 *a5 = new Ais5(body.c_str(), fillbits);

        AIS_PARSER::Vessel* v = AIS_PARSER::FindVesselByMMSI(a5->mmsi);
        if (nullptr == v)
        {
            v = new AIS_PARSER::Vessel(a5);
            v->mmsi = a5->mmsi;
            v->callsign = a5->callsign;
            v->name = a5->name;
            v->type_and_cargo = a5->type_and_cargo;
            v->destination = a5->destination;
            VesselList.push_back(v);
        }
        else //just update the thing
        {
            v->ais5 = a5;
            v->callsign = a5->callsign;
            v->name = a5->name;
            v->type_and_cargo = a5->type_and_cargo;
            v->destination = a5->destination;
            v->age = 0;
        }
        return (AIS_PARSER::AISObject*)v;
    }
    return nullptr;
}


AIS_PARSER::AISObject* AIS_PARSER::ParseASI24IdentPayload(std::string body, int fillbits)
{
    std::unique_ptr<libais::AisMsg>  p = CreateAisMsg(body, 0);
    if (nullptr == p)
    {
        std::cout << "Null ptr" << std::endl;
        return nullptr;
    }
    else
    {
        Ais24* a24 = new Ais24(body.c_str(), fillbits);

        AIS_PARSER::Vessel* v = AIS_PARSER::FindVesselByMMSI(a24->mmsi);
        if (nullptr == v)
        {
            v = new AIS_PARSER::Vessel(a24);
            v->mmsi = a24->mmsi;
            v->callsign = a24->callsign;
            v->name = a24->name;
            v->type_and_cargo = a24->type_and_cargo;
            //v->destination = a24->destination;
            VesselList.push_back(v);
        }
        else //just update the thing
        {
            v->ais24 = a24;
            v->callsign = a24->callsign;
            v->name = a24->name;
            v->type_and_cargo = a24->type_and_cargo;
            //v->destination = a24->destination;
            v->age = 0;
        }
        return (AIS_PARSER::AISObject*)v;
    }
    return nullptr;
}




AIS_PARSER::AISObject * AIS_PARSER::ParseAIS123_PosReportPayload(std::string body, int fillbits)
{
       std::unique_ptr<libais::AisMsg>  p = CreateAisMsg(body, fillbits);
    if (nullptr == p)
    {
        std::cout << "Null ptr" << std::endl;
        return nullptr;
    }
    else
    {
        Ais1_2_3 *a123 =  new Ais1_2_3(body.c_str(), 0);

        /*
        std::stringstream retVal{};
        //std::unique_ptr<Ais1_2_3> a123 = std::unique_ptr<Ais1_2_3>(new Ais1_2_3(body.c_str(), 0));
        retVal << "ParsePosReportPayload:"  << std::endl;
        retVal << "user ID " << a123->mmsi << std::endl;
        retVal << "nav_status " << NAV_STATUS[a123->nav_status] << std::endl;
        retVal << "true_heading " << a123->true_heading << std::endl;
        retVal << "position, lat " << a123->position.lat_deg << std::endl;
        retVal << "position, lng " << a123->position.lng_deg << std::endl;
        retVal << "time stamp " << a123->timestamp << std::endl;

        //wxLogMessage(retVal.str());
        */

        AIS_PARSER::Vessel* v = AIS_PARSER::FindVesselByMMSI(a123->mmsi);
        if (nullptr == v)
        {
            v = new AIS_PARSER::Vessel(a123);
            v->mmsi = a123->mmsi;
            v->nav_status = a123->nav_status;
            v->true_heading = a123->true_heading;
            v->lat_deg = a123->position.lat_deg;
            v->lng_deg = a123->position.lng_deg;
            v->timestamp = a123->timestamp;


            VesselList.push_back(v);
        }
        else //just update the thing
        {
            v->a123 = a123;

            v->nav_status = a123->nav_status;
            v->true_heading = a123->true_heading;
            v->lat_deg = a123->position.lat_deg;
            v->lng_deg = a123->position.lng_deg;
            v->timestamp = a123->timestamp;
            v->age = 0;
        }
        return (AIS_PARSER::AISObject *)v;
    }
    return nullptr;

}


AIS_PARSER::AISObject * AIS_PARSER::ParseAIS18_PosReportPayload(std::string body, int fillbits)
{
    std::stringstream retVal{};

    std::unique_ptr<libais::AisMsg>  p = CreateAisMsg(body, fillbits);
    if (nullptr == p)
    {
        std::cout << "Null ptr" << std::endl;
        return nullptr;
    }
    else
    {
        Ais18* a18 = new Ais18(body.c_str(), 0);

        //std::unique_ptr<Ais1_2_3> a123 = std::unique_ptr<Ais1_2_3>(new Ais1_2_3(body.c_str(), 0));
        retVal << "ParsePosReportPayload:" << std::endl;
        retVal << "user ID " << a18->mmsi << std::endl;
        //retVal << "nav_status " << NAV_STATUS[a18->nav_status] << std::endl;
        retVal << "true_heading " << a18->true_heading << std::endl;
        retVal << "position, lat " << a18->position.lat_deg << std::endl;
        retVal << "position, lng " << a18->position.lng_deg << std::endl;
        retVal << "time stamp " << a18->timestamp << std::endl;

        //wxLogMessage(retVal.str().c_str());

        AIS_PARSER::Vessel* v = AIS_PARSER::FindVesselByMMSI(a18->mmsi);
        if (nullptr == v)
        {
            v = new AIS_PARSER::Vessel(a18);
            v->mmsi = a18->mmsi;
            //v->nav_status = -1;// a18->nav_status;
            v->true_heading = a18->true_heading;
            v->lat_deg = a18->position.lat_deg;
            v->lng_deg = a18->position.lng_deg;
            v->timestamp = a18->timestamp;


            VesselList.push_back(v);
        }
        else //just update the thing
        {
            v->a18= a18;

            //v->nav_status = a18->nav_status;
            v->true_heading = a18->true_heading;
            v->lat_deg = a18->position.lat_deg;
            v->lng_deg = a18->position.lng_deg;
            v->timestamp = a18->timestamp;
            v->age = 0;
        }
        return (AIS_PARSER::AISObject*)v;
    }
    return nullptr;

}
