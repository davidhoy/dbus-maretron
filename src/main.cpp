#include <stdio.h>
#include <cstdlib>
#include <stdio.h>
#include <iostream>

char socketName[50];	
#define SOCKET_CAN_PORT socketName
#include <NMEA2000_CAN.h>
#include <N2kDeviceList.h>
#include <N2kMessages.h>

//#include <dbus/dbus.h>

using namespace std;

// List here messages your device will transmit.
const unsigned long TransmitMessages[] PROGMEM={130306L,0};

tN2kDeviceList *pN2kDeviceList;


// *****************************************************************************
// Call back for NMEA2000 open. This will be called, when library starts bus communication.
// See NMEA2000.SetOnOpen(OnN2kOpen); in main()
void OnN2kOpen() {
  // Start schedulers now.
 // WindScheduler.UpdateNextTime();
}


//*****************************************************************************
void PrintUlongList(const char *prefix, const unsigned long * List) 
{
  uint8_t i;
  if ( List!=0 ) {
    printf("%s", prefix);
    for (i=0; List[i]!=0; i++) { 
      if (i>0) printf(", ");
      printf("%d", List[i]);
    }
    printf("\n");
  }
}

//*****************************************************************************
void PrintText(const char *Text, bool AddLineFeed=true)
 {
  if ( Text!=0 )
  	printf("%s", Text);
  if ( AddLineFeed ) 
  	printf("\n");   
}

//*****************************************************************************
void PrintDevice(const tNMEA2000::tDevice *pDevice)
 {
  	if (pDevice == 0) 
  		return;
 
  	printf("----------------------------------------------------------------------\n");
  	printf("%s\n", pDevice->GetModelID());
  	printf("  Source: %d\n", pDevice->GetSource());
  	printf("  Manufacturer code:        %d\n", pDevice->GetManufacturerCode());
  	printf("  Unique number:            %d\n", pDevice->GetUniqueNumber());
  	printf("  Software version:         %s\n", pDevice->GetSwCode());
  	printf("  Model version:            %s\n", pDevice->GetModelVersion());
  	printf("  Manufacturer Information: %s\n", pDevice->GetManufacturerInformation());
  	printf("  Installation description1: %s\n", pDevice->GetInstallationDescription1());
  	printf("  Installation description2: %s\n", pDevice->GetInstallationDescription2());
  	PrintUlongList("  Transmit PGNs :", pDevice->GetTransmitPGNs());
  	PrintUlongList("  Receive PGNs  :", pDevice->GetReceivePGNs());
  	printf("\n");;
}


#define START_DELAY_IN_S 8
//*****************************************************************************
void ListDevices(bool force = false)
{
  	static bool StartDelayDone=false;
  	static int StartDelayCount=0;
  	static unsigned long NextStartDelay=0;
  
  	if (!StartDelayDone) { // We let system first collect data to avoid printing all changes
    	if (millis() > NextStartDelay) {
      		if (StartDelayCount == 0) {
        		printf("Reading device information from bus ");
        		NextStartDelay = millis();
      		}
      		printf(".");
      		NextStartDelay += 1000;
      		StartDelayCount++;
      		if (StartDelayCount > START_DELAY_IN_S) {
        		StartDelayDone = true;
        		printf("\n");
      		}
    	}
    	return;
  	}
  	if (!force && !pN2kDeviceList->ReadResetIsListUpdated()) 
		return;
 
  	printf("\n");
  	printf("**********************************************************************");
  	for (uint8_t i = 0; i < N2kMaxBusDevices; i++) 
		PrintDevice(pN2kDeviceList->FindDeviceBySource(i));
}


void DcDetailedStatusHandler(const tN2kMsg &N2kMsg) 
{
	//printf("Received PGN 127506, DC Detailed Status\n");

	unsigned char	SID;
	unsigned char 	DCInstance;
	tN2kDCType 		DCType;
	unsigned char	StateOfCharge;
	unsigned char 	StateOfHealth;
	double 			TimeRemaining;
	double 			RippleVoltage;
	double			Capacity;

	bool ret = ParseN2kDCStatus(N2kMsg, SID, DCInstance, DCType, StateOfCharge, StateOfHealth, TimeRemaining, RippleVoltage, Capacity);
	if (ret) {
		printf("DcDetailedStatus: SID=%d, inst=%d, type=%d, SOC=%d%%, SOH=%d%%, remaining=%.1f, ripple=%.2fV capacity=%.2fAh\n", 
				SID, DCInstance, DCType, StateOfCharge, StateOfHealth, TimeRemaining/60, RippleVoltage, Capacity);
	}
}

void BatteryStatusHandler(const tN2kMsg &N2kMsg)
{
	//printf("Received PGN 127508, Battery Status\n");

	unsigned char	BatteryInstance;
	double 			BatteryVoltage;
	double			BatteryCurrent;
    double			BatteryTemperature;
	unsigned char	SID;

	bool ret = ParseN2kDCBatStatus(N2kMsg, BatteryInstance, BatteryVoltage, BatteryCurrent, BatteryTemperature, SID);
	if (ret ) {
		printf("BatteryStatus: instance=%d, voltage=%.2fV, current=%.2fA, temp=%.1fdeg, seq=%d\n",
				BatteryInstance, BatteryVoltage, BatteryCurrent, BatteryTemperature, SID);
	}
}

void DcCurrentVoltageHandler(const tN2kMsg &N2kMsg) 
{
	printf("Received PGN 127751, DC Current/Voltage Status\n");
}


typedef struct {
  	unsigned long PGN;
  	void (*Handler)(const tN2kMsg &N2kMsg); 
} tNMEA2000Handler;


tNMEA2000Handler NMEA2000Handlers[] = {
  { 127506L, &DcDetailedStatusHandler },
  { 127508L, &BatteryStatusHandler    },
  { 127751L, &DcCurrentVoltageHandler },
  { 0,       NULL					  }
};


//NMEA 2000 message handler
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg) {
  	int iHandler;
  
  	// Find handler
	//printf("Received PGN %d, looking for handler...", N2kMsg.PGN);
  	for (iHandler = 0; NMEA2000Handlers[iHandler].PGN != 0 && !(N2kMsg.PGN == NMEA2000Handlers[iHandler].PGN); iHandler++)
		;
	
  	if (NMEA2000Handlers[iHandler].PGN != 0) {
		//printf("found handler, calling it\n");
    	NMEA2000Handlers[iHandler].Handler(N2kMsg); 
  	} else {
		//printf("no handler\n");
	}
}


int main(int argc, char** argv)
{
	setvbuf(stdout, NULL, _IONBF, 0);                                          // No buffering on stdout, just send chars as they come.

#if 0
	DBusError error;
    dbus_error_init(&error);

    DBusConnection *connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    if (!connection || dbus_error_is_set(&error)) {
        perror("Unable to establish dbus connection.");
        exit(1);
    }

	int ret = dbus_bus_request_name(connection, "com.maretron.dbus", DBUS_NAME_FLAG_REPLACE_EXISTING, &error);
    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER || dbus_error_is_set(&error)) {
        perror("Unable to request dbus request name.");
        exit(1);
    }
#endif
 
    //NMEA2000.SetForwardStream(&serStream);                                      // Connect bridge function for streaming output.
    NMEA2000.SetForwardType(tNMEA2000::fwdt_Text);  


	// Set product information
	const char ModelSerialCode[32] = "12345678";
	NMEA2000.SetProductInformation(ModelSerialCode, // Manufacturer's Model serial code
                                   247, 			// Manufacturer's product code
                                   "dbus-maretron",	// Manufacturer's Model ID
                                   "1.0.0.0",  		// Manufacturer's Software version code
                                   "1.0.0.0" 		// Manufacturer's Model version
                                 );

  	// Set device information
  	const unsigned long SerialNumber = 87654321;
  	NMEA2000.SetDeviceInformation(SerialNumber,		// Unique number. Use e.g. Serial number.
                                130, 				// Device function=PC Gateway. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                25, 				// Device class=Inter/Intranetwork Device. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                137, 				// Just choosen free from code list on https://web.archive.org/web/20190529161431/http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
								4,					// Industry group, 4=Marine
								0					// iDev
							   );

	// If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
  	NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, 50);
  

    NMEA2000.SetForwardStream(0); //&Serial);
	NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); 	// Show in clear text. Leave uncommented for default Actisense format.
  	NMEA2000.EnableForward(false);
  	
	// Here we tell library, which PGNs we transmit
  	NMEA2000.ExtendTransmitMessages(TransmitMessages);
  
  	// Define OnOpen call back. This will be called, when CAN is open and system starts address claiming.
  	NMEA2000.SetOnOpen(OnN2kOpen);

	pN2kDeviceList = new tN2kDeviceList(&NMEA2000);

	NMEA2000.SetMsgHandler(HandleNMEA2000Msg);

	strcpy(socketName,"can0");						// Populate string 'socketName' BEFORE calling  NMEA2000.Open();
	if (!NMEA2000.Open()) {
		printf("Failed to open CAN port: %s\n", socketName);
		return 1;
	}

	while(1) {
         NMEA2000.ParseMessages();					// Will send out CAN messages in open text 
		 //ListDevices(false);
    }

	return 0;
}
