#include "S3B.h"

// #include "NCD16Relay/NCD16Relay.h"

#include "application.h"

// NCD16Relay relay;

void S3B::init(){
	pinMode(sleepPin, OUTPUT);
	pinMode(sleepStatus, INPUT);
	digitalWrite(sleepPin, LOW);
}

bool S3B::wake(){
	digitalWrite(sleepPin, LOW);
	unsigned long start = millis();
	while((analogRead(sleepStatus) < 900) && millis() < start+sleepWakeTimeOut);
	if(analogRead(sleepStatus) < 900){
//	    Serial1.println("Wake failed");
//	    Serial1.printf("analog reading: %i \n",analogRead(sleepStatus));
	    return false;
	}else{
	    return true;
	}
}

bool S3B::sleep(){
	digitalWrite(sleepPin, HIGH);
	unsigned long start = millis();
	while((analogRead(sleepStatus) > 400) && millis() < start+sleepWakeTimeOut);
    if(analogRead(sleepStatus) > 400){
        Serial.println("Sleep failed");
        return false;
    }else{
        return true;
    }
}

bool S3B::isAwake(){
    return(analogRead(sleepStatus) > 2000);
}

bool S3B::transmit(byte *address, byte *data, int len){
//	Serial.println("Serial1.begin in S3B.transmit");
	Serial1.begin(115200);
//	Serial.println("Serial1.begin complete in S3B.transmit");
    int tlen = len+14;
    byte payload[len+18];
    // len = len+14;

    byte lsb = (tlen & 255);
    byte msb = (tlen >> 8);

    // Serial.printf("MSB: %i \n", msb);
    // Serial.printf("LSB: %i \n", lsb);

    //build the packet
    payload[0] = startDelimiter;
    payload[1] = msb;
    payload[2] = lsb;
    payload[3] = frameType;
    payload[4] = 0;
    //populate address in packet
    for(int i = 0; i < 8; i++){
        payload[5+i] = address[i];
    }
    payload[13] = reserved1;
    payload[14] = reserved2;
    payload[15] = bRadius;
    payload[16] = transmitOptions;
    //populate data in packet
    for(int i = 0; i < len; i++){
        payload[17+i] = data[i];
    }
    int c = 0;
    for(int i = 3; i < len+17; i++){
        c += payload[i];
    }
    byte checksum = 0xFF - (c & 0xFF);
    payload[len+17] = checksum;
//    Serial.println("Serial1.write in S3B.transmit");
    Serial1.write(payload, len+18);
//    Serial.println("Serial1.write complete in S3B.transmit");
    // Serial.print("Sending: ");
    // for(int i = 0; i < len+18; i++){
    //     Serial.printf("%x, ", payload[i]);
    // }
    // Serial.println();
//    restart:

    unsigned long start = millis();
    // delay(200);
    // while(Serial1.available() != 0){
    //     Serial.printf("%x, ", Serial1.read());
    // }
    // Serial.println();

    while(Serial1.available() == 0 && millis() < start + ackTimeOut);
    if(Serial1.available() == 0){
        Serial.println("Timeout waiting for ack of transmission to slave");
        delay(50);
        flushSerialPort();
//        delay(2000);
        return false;
    }
    int s = Serial1.read();
    if(s != 0x7E){
        Serial.println("First byte returned is not start delimiter for valid packet");
        Serial.printf("It was: %x \n", s);
        flushSerialPort();
        return false;
    }
    while(Serial1.available() < 2 && millis() < start + ackTimeOut);
    byte MSB = Serial1.read();
    byte LSB = Serial1.read();
    int totalPacketLength = ((MSB * 256)+LSB)+1;
    while(Serial1.available() != totalPacketLength && millis() < start + ackTimeOut);
    if(Serial1.available() != totalPacketLength){
        Serial.println("Full Packet not received");
        delay(50);
        flushSerialPort();
        return 0;
    }
    byte packetData[totalPacketLength];
//    Serial.print("Received: ");
    for(int i = 0; i < totalPacketLength; i++){
        packetData[i] = Serial1.read();
//        Serial.printf("%x, ", packetData[i]);
    }
//    Serial.println();
    if(packetData[0] == 0x8B){
        if(packetData[5] == 0x00){


            if(frameID < 255){
            		frameID++;
            	}else{
            		frameID = 1;
            	}
            Serial.println("Here");

            	return true;
        }else{
            Serial.printf("Bad response code, it was %x \n", packetData[5]);
            return false;
        }
    }
    if(packetData[0] == 0x90){
//        Serial.println("got data back from slave");

        byte newPacketData[totalPacketLength + 3];
        newPacketData[0] = s;
        newPacketData[1] = MSB;
        newPacketData[2] = LSB;
        for(int i = 0; i < totalPacketLength; i++){
            newPacketData[i+3] = packetData[i];
        }
//        Serial.print("Received: ");
//        for(int i = 0; i < totalPacketLength+3; i++){
//        	Serial.printf("%x, ", newPacketData[i]);
//        }
//        Serial.println();
        updateLocalRelays(newPacketData, totalPacketLength + 3);

        return true;

    }
    Serial.println("Should not have gotten here");
    return false;
}

bool S3B::parseAddress(String addr, byte *buffer){
    int ind = 0;
    int pos = addr.indexOf('.');
    while(pos>-1){
        buffer[ind] = addr.substring(0, pos).toInt();
        addr = addr.substring(pos+1);
        pos = addr.indexOf('.');
        ind++;
    }
    buffer[ind] = addr.toInt();
    return true;
}

bool S3B::validateReceivedData(byte *s3bData, int len){
    if(len < 16){
        Serial.println("Invalid packet, too short");
        return false;
    }
    int c = 0;
    for(int i = 3; i < len -1; i++){
        c += s3bData[i];
    }
    byte checksum = 0xFF - (c & 0xFF);
    if(s3bData[len-1] == checksum){
        return true;
    }else{
        Serial.println("Invalid packet, checksum incorrect");
        return false;
    }
}

int S3B::getReceiveDataLength(byte *s3bData){
    int length = ((s3bData[1]*256)+s3bData[2])-12;
    return length;
}

int S3B::parseReceive(byte *s3bData, char *buffer, int len, byte *senderAddress){
    for(int i = 0; i < 8; i++){
        senderAddress[i] = s3bData[i+4];
    }
    int count = 0;
    for(int i = 15; i < len-1; i++){
        buffer[i-15] = (char)s3bData[i];
        count++;
    }
    return count;
}

void S3B::flashLED(int count){
	for(int i = 0; i < count; i++){
		digitalWrite(D7, HIGH);
		delay(200);
		digitalWrite(D7, LOW);
		delay(200);
	}
}

void S3B::flushSerialPort(){
    while(Serial1.available() != 0){
        Serial1.read();
    }
}

int S3B::getRSSI(){
	Serial1.begin(115200);
	//Create byte arrays to hold command.  Second one holds full command plus checksum.
	byte getRSSICommand[8] = {0x7E, 0x00, 0x04, 0x08, 0x5A, 0x44, 0x42, 0x17};

	Serial1.write(getRSSICommand, 8);

	//Timeout Parameters.
	unsigned long rssiReadTimeout = 300;
	unsigned long rssiStartTime = millis();

	//Wait for response from module or timeout after 300 mS.
	while(Serial1.available() == 0 && millis() < rssiStartTime + rssiReadTimeout);

	//If nothing returned from module.  Error.
	if(Serial1.available() == 0){
		Serial.println("No Return from Module");
		return 0;
	}else{
		//Read RSSI byte back from module
		int commandResponse[10];
		commandResponse[0] = Serial1.read();
		if(commandResponse[0] != 0x7e){
			return 256;
		}
		while(Serial1.available() < 2 && millis() < rssiStartTime + rssiReadTimeout);
		if(Serial1.available() < 2){
			return 256;
		}
		commandResponse[1] = Serial1.read();
		commandResponse[2] = Serial1.read();
		int len = (commandResponse[1]*256)+commandResponse[2];
		if(len != 6){
			return 256;
		}
		while(Serial1.available() < 7 && millis() < rssiStartTime + rssiReadTimeout);
		if(Serial1.available() < 7){
			return 256;
		}
		for(int i = 3; i < 10; i++){
			commandResponse[i] = Serial1.read();
		}
		int rssi = commandResponse[8];
		rssi = 0-rssi;
		return rssi;
	}
}

