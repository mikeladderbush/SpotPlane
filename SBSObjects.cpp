#include "SBSObjects.h"
#include <string>
#include <sstream>
#include <vector>

AircraftUpdate ParseSBS(const std::string& payload) {
	// Field names vary in format to exactly match the source: http://woodair.net/sbs/article/barebones42_socket_data.htm
	std::stringstream ss(payload);
	std::string field;
	std::vector <std::string> fields;

	auto get = [&](int i) -> std::string {
		return i < (int)fields.size() ? fields[i] : "";
	};

	AircraftUpdate update;
	update.Message_type = get(0);
	update.Transmission_Type = get(1);
	update.Session_ID = get(2);
	update.AircraftID = get(3);
	update.HexIdent = get(4);
	update.FlightID = get(5);
	update.Date_message_generated = get(6);
	update.Time_message_generated = get(7);
	update.Date_message_logged = get(8);
	update.Time_message_logged = get(9);
	update.Callsign = get(10);
	update.Altitude = get(11);
	update.GroundSpeed = get(12);
	update.Track = get(13);
	update.Latitude = get(14);
	update.Longitude = get(15);
	update.VerticalRate = get(16);
	update.Squawk = get(17);
	update.Alert = get(18);
	update.Emergency = get(19);
	update.SPI = get(20);
	update.IsOnGround = get(21);

	return update;
}