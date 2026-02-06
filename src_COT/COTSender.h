#pragma once

#include "bg_TakMessage.h"


namespace COTSENDER
{
	std::string StartCOTSender();
	void StopCOTSender();
	std::string SendCoTMsg(bg_TakMessage CurCoTMsg);

}
