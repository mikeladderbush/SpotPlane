#pragma once

#include <string>

struct AircraftUpdate
{
	// These are standard for all messages.
	std::string Message_type;
	// Transmission type only applies to MSG sub-types.
	std::string Transmission_Type;
	std::string Session_ID;
	std::string AircraftID;
	std::string HexIdent;
	std::string FlightID;
	std::string Date_message_generated;
	std::string Time_message_generated;
	std::string Date_message_logged;
	std::string Time_message_logged;

	// The following are aircraft specific.
	std::string Callsign;
	std::string Altitude;
	std::string GroundSpeed;
	std::string Track;
	std::string Latitude;
	std::string Longitude;
	std::string VerticalRate;
	std::string Squawk;
	std::string Alert;
	std::string Emergency;
	std::string SPI;
	std::string IsOnGround;

};

AircraftUpdate ParseSBS(const std::string& payload);