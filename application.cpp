// This #include statement was automatically added by the Particle IDE.
#include "MCP23017_16_IO.h"

// This #include statement was automatically added by the Particle IDE.
#include "NCD16Relay.h"

// This #include statement was automatically added by the Particle IDE.
#include "S3B.h"

#include "math.h"

#include "application.h"

#include "softap_http.h"

// STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));

SYSTEM_MODE(MANUAL);


NCD16Relay relayController;
S3B module;
MCP23017_16_IO inputBoard;

int numberOfSlaves = 8;

//0013a20040f672a5
//0013a20040b377d8
//0013a20040f672bb
//0013a20040f672a1
//0013a20040f672a6
//0013a20040f27479
//0013a20040f672a2
//0013a20041565149

byte slaveAddress[8][8];
byte blankAddress[8] = {255,255,255,255,255,255,255,255};

String device;

char stringSlaveAddress[8][16];

byte receiveBuffer[256];
unsigned long tOut = 3000;

int inputStatus[2];
int previousInputStatus[2];

bool readInputs();
void updateLocalRelays(byte* packet, int packetLen);
void updateSlaveStatus();
void getInfoFromMemory();

unsigned long lastHeard[8] = {0,0,0,0,0,0,0,0};
unsigned long slaveTimeout = 10000;
unsigned long slaveRetryInterval = 20000;
byte deviceStatus[8] = {1,1,1,1,1,1,1,1};
unsigned long deviceFailTime[8] = {0,0,0,0,0,0,0,0};
int deviceRSSI[8] = {0,0,0,0,0,0,0,0};
unsigned long deviceStartTime[8] = {0,0,0,0,0,0,0,0};
int deviceSuccessCount[8] = {0,0,0,0,0,0,0,0};
int deviceFailCount[8] = {0,0,0,0,0,0,0,0};

int roundTrip = 0;

struct Page
{
	const char* url;
	const char* mime_type;
	const char* data;
};


const char index_html[] = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>Configure Slaves</title><link rel='stylesheet' type='text/css' href='styles/drag.css'><script type='text/javascript' src='/scripts/devices.js'></script></head><body><form><fieldset id='devices' class='dragster-region'></fieldset><input type='button' id='addnew' value='Add New' /><input type='submit' value='Submit' /></form><script type='text/javascript' src='/scripts/drag.js'></script><script type='text/javascript' src='/scripts/init.js'></script></body></html>";

const char init_js[] = "(function() {'use strict';function initDrag() {new window.Dragster({  elementSelector: '.dragster-block',  regionSelector: '.dragster-region',});};var systemLatency = document.createElement('span');systemLatency.textContent = 'System Latency: ' + latency + ' milliSeconds';var masterBlock = document.createElement('div');masterBlock.appendChild(systemLatency);document.getElementById('devices').appendChild(masterBlock);var slave = 1;var rssiS = rssi.split(',');var slaveFailSuccessS = failSuccessCount.split('|');var upTimeS = deviceUpTime.split(',');for (var i = 0; i < devices.length; i += 16) {var slaveValue = devices.substring(i, i + 16);var slaveStatus = 1 /*devices.substring(i+8, i+9)*/ ;var slaveInput = document.createElement('input');slaveInput.name = 'slave_' + slave;slaveInput.value = slaveValue;slaveInput.onclick = function() {  this.focus()};var slaveName = document.createElement('span');slaveName.textContent = 'Slave ' + slave;var slaveRSSI = document.createElement('span');if(rssiS[slave -1] == '256'){slaveRSSI.textContent = 'Offline';}else{slaveRSSI.textContent = 'dBm: ' + rssiS[slave -1];}var slaveUpTimeContainer = document.createElement('div');var slaveUpTime = document.createElement('span');slaveUpTime.textContent = 'Up Time: ' + upTimeS[slave -1];slaveUpTimeContainer.appendChild(slaveUpTime);var slaveFailSuccessSS = slaveFailSuccessS[slave -1].split(',');var slaveTransCountContainer = document.createElement('div');var slaveFailCounter = document.createElement('span');var slaveSuccessCounter = document.createElement('span');slaveFailCounter.textContent = 'Failures: '+slaveFailSuccessSS[0];slaveSuccessCounter.textContent = '     Transmissions: ' +slaveFailSuccessSS[1];slaveTransCountContainer.appendChild(slaveFailCounter);slaveTransCountContainer.appendChild(slaveSuccessCounter);var slaveBlock = document.createElement('div');slaveBlock.className = 'dragster-block' + (slaveStatus == 1 ? ' connected' : ' error');slaveBlock.appendChild(slaveName);slaveBlock.appendChild(slaveInput);slaveBlock.appendChild(slaveRSSI);slaveBlock.appendChild(slaveUpTimeContainer);slaveBlock.appendChild(slaveTransCountContainer);document.getElementById('devices').appendChild(slaveBlock);slave++;};var newI = 0;document.getElementById('addnew').onclick = function() {newI++;var newInput = document.createElement('input');newInput.name = 'new-' + newI;newInput.onclick = function() {  this.focus()};var newBlock = document.createElement('div');newBlock.className = 'dragster-block';var newName = document.createElement('span');newName.textContent = 'New';newBlock.appendChild(newName);newBlock.appendChild(newInput);document.getElementById('devices').appendChild(newBlock);};initDrag();})();";

//const char init_js[] = "(function () {'use strict';function initDrag(){new window.Dragster({elementSelector: '.dragster-block',regionSelector: '.dragster-region',});};var slave=1;for(var i=0;i<devices.length;i+=16){var slaveValue=devices.substring(i, i+16);var slaveStatus=1/*devices.substring(i+8, i+9)*/;var slaveInput=document.createElement('input');slaveInput.name='slave_'+slave;slaveInput.value=slaveValue;slaveInput.onclick=function(){this.focus()};var slaveName=document.createElement('span');slaveName.textContent='Slave '+slave;var slaveBlock=document.createElement('div');slaveBlock.className='dragster-block'+(slaveStatus==1?' connected':' error');slaveBlock.appendChild(slaveInput);slaveBlock.appendChild(slaveName);document.getElementById('devices').appendChild(slaveBlock);slave++;};var newI=0;document.getElementById('addnew').onclick=function(){newI++;var newInput=document.createElement('input');newInput.name='new-'+newI;newInput.onclick=function(){this.focus()};var newBlock=document.createElement('div');newBlock.className='dragster-block';var newName=document.createElement('span');newName.textContent='New';newBlock.appendChild(newName);newBlock.appendChild(newInput);document.getElementById('devices').appendChild(newBlock);};initDrag();})();";

const char drag_js[] = "(function(e,t){'use strict';e.Dragster=function(n){var r='is-dragging',o='is-drag-over',s='dragster-draggable',i='dragster-drag-region',a='dragster-drop-placeholder',l='dragster-temp',d='dragster-temp-container',c='dragster-is-hidden',f='dragster-replacable',u='touchstart',p='touchmove',m='touchend',g='mousedown',h='mousemove',v='mouseup',E='top',L='bottom',y='px',C='div',B=false,w=true,A=function(){},b={elementSelector:'.dragster-block',regionSelector:'.dragster-region',replaceElements:B,updateRegionsHeight:w,minimumRegionHeight:50,onBeforeDragStart:A,onAfterDragStart:A,onBeforeDragMove:A,onAfterDragMove:A,onBeforeDragEnd:A,onAfterDragEnd:A},N='draggable',D='data-placeholder-position',H={top:B,bottom:B},S={drag:{node:{}},drop:{node:{}},shadow:{node:{},top:0,left:0},placeholder:{node:{},position:''}},T,M,R,q,O,X,Y,x,I,P,k,F,j,z,G,J,K,Q,U,V,W,Z,$,_,ee;for(M in n){if(n.hasOwnProperty(M)){b[M]=n[M]}}$=function(){return Array.prototype.slice.call(t.querySelectorAll(b.elementSelector))};_=function(e){e.forEach(function(e){var t=J(),n=e.parentNode;if(n.classList.contains(s)){return B}n.insertBefore(t,e);n.removeChild(e);t.appendChild(e)})};I=$();R=Array.prototype.slice.call(t.querySelectorAll(b.regionSelector));if(b.replaceElements){Y=t.createElement(C);Y.classList.add(c);Y.classList.add(d);t.body.appendChild(Y)}q=function(e,t){var n=e.parentNode;if(!n||e.classList&&e.classList.contains(i)){return undefined}if(t(e)){return e}return t(n)?n:q(n,t)};V=function(e){var n=Array.prototype.slice.call(t.getElementsByClassName(e));n.forEach(function(e){e.parentNode.removeChild(e)})};W=function(e,n){R.forEach(function(e){e.removeEventListener(n,P.mousemove)});t.body.removeEventListener(n,P.mousemove);if(e){e.classList.remove(r)}Array.prototype.slice.call(t.getElementsByClassName(s)).forEach(function(e){if(!e.firstChild){e.parentNode.removeChild(e)}});V(a);V(l);ee()};Z=function(){Array.prototype.slice.call(t.getElementsByClassName(f)).forEach(function(e){e.classList.remove(f)})};J=function(){var e=t.createElement(C);e.setAttribute(N,w);e.classList.add(s);return e};Q=function(){var e=t.createElement(C);e.classList.add(a);return e};K=function(){var e=t.createElement(C);e.classList.add(l);e.classList.add(c);t.body.appendChild(e);return e};z=function(e,t){if(e&&e.parentNode){e.parentNode.insertBefore(t,e.nextSibling)}};G=function(e,t){if(e&&e.parentNode){e.parentNode.insertBefore(t,e)}};j=function(e){return e.classList&&e.classList.contains(i)};F=function(e){return e.classList&&e.classList.contains(s)};k=function(e){return e.classList&&e.classList.contains(a)};ee=function(){if(b.updateRegionsHeight){var n=Array.prototype.slice.call(t.getElementsByClassName(i));n.forEach(function(t){var n=Array.prototype.slice.call(t.querySelectorAll(b.elementSelector)),r=b.minimumRegionHeight;if(n.length){n.forEach(function(t){var n=e.getComputedStyle(t);r+=t.offsetHeight+parseInt(n.marginTop,10)+parseInt(n.marginBottom,10)})}t.style.height=r+y})}};P={mousedown:function(e){e.dragster=T;if(b.onBeforeDragStart(e)===B||e.which===3){return B}e.preventDefault();var n,o=e.type===u?p:h;R.forEach(function(e){e.addEventListener(o,P.mousemove)});t.body.addEventListener(o,P.mousemove);x=q(e.target,F);if(!x){return B}n=x.getBoundingClientRect();O=K();O.innerHTML=x.innerHTML;O.style.width=n.width+y;O.style.height=n.height+y;X=O.getBoundingClientRect();x.classList.add(r);T=S;T.drag.node=x;T.shadow.node=O;e.dragster=T;b.onAfterDragStart(e)},mousemove:function(e){e.dragster=T;if(b.onBeforeDragMove(e)===B){return B}e.preventDefault();var n=e.changedTouches?e.changedTouches[0]:e,r=n.view?n.view.pageXOffset:0,o=n.view?n.view.pageYOffset:0,l=n.clientY+o,d=n.clientX+r,u=t.elementFromPoint(n.clientX,n.clientY),p=q(u,F),m=l+25,g=d-X.width/2,h=Q(),v,C;clearTimeout(U);O.style.top=m+y;O.style.left=g+y;O.classList.remove(c);T.shadow.top=m;T.shadow.left=g;if(p&&p!==x){v=p.getBoundingClientRect();C=v.height/2;Z();if(!b.replaceElements){if(l-v.top<C&&!H.top){V(a);h.setAttribute(D,E);G(p.firstChild,h);T.placeholder.position=E}else if(v.bottom-l<C&&!H.bottom){V(a);h.setAttribute(D,L);p.appendChild(h);T.placeholder.position=L}}else{p.classList.add(f)}T.placeholder.node=h;T.drop.node=p}else if(u.classList.contains(i)&&u.getElementsByClassName(s).length===0&&u.getElementsByClassName(a).length===0){u.appendChild(h);T.placeholder.position=L;T.placeholder.node=h;T.drop.node=u}else if(u.classList.contains(i)&&u.getElementsByClassName(s).length>0&&u.getElementsByClassName(a).length===0){var w=u.getElementsByClassName(s);p=w[w.length-1];h.setAttribute(D,L);V(a);p.appendChild(h);T.placeholder.position=L;T.placeholder.node=h;T.drop.node=p}else if(!u.classList.contains(i)){if(!b.replaceElements){V(a)}else{Z()}}ee();b.onAfterDragMove(e)},mouseup:function(e){e.dragster=T;if(b.onBeforeDragEnd(e)===B){return B}var n=b.replaceElements?f:a,r=t.getElementsByClassName(n)[0],s,i,l=e.type===u?p:h,c;U=setTimeout(function(){W(x,l)},200);Z();if(!x||!r){W(x,l);return B}s=q(r,F);s=s||r;if(x!==s){if(!b.replaceElements){c=J();i=r.getAttribute(D);if(i===E){G(s,c)}else{z(s,c)}if(x.firstChild){c.appendChild(x.firstChild)}}else{c=t.getElementsByClassName(d)[0];c.innerHTML=x.innerHTML;x.innerHTML=s.innerHTML;s.innerHTML=c.innerHTML;c.innerHTML=''}s.classList.remove(o)}W(x,l);b.onAfterDragEnd(e)}};_(I);t.body.addEventListener(v,P.mouseup,B);t.body.addEventListener(m,P.mouseup,B);R.forEach(function(e){e.classList.add(i);e.addEventListener(g,P.mousedown,B);e.addEventListener(v,P.mouseup,B);e.addEventListener(u,P.mousedown,B);e.addEventListener(m,P.mouseup,B)});return{update:function(){_($());ee()}}}})(window,window.document);";

const char drag_css[] = "*{box-sizing:border-box}body{width:100%;max-width:60rem;margin:auto;font:100%/1.4 Arial,sans-serif;text-align:center}[draggable]{-moz-user-select:none;-khtml-user-select:none;-webkit-user-select:none;-ms-user-select:none;user-select:none;-khtml-user-drag:element;-webkit-user-drag:element}.dragster-region{height: inherit !important;display:inline-block;outline:#00f solid 1px;padding:1rem;width:45%;vertical-align:top;min-height:3rem}.dragster-region+.dragster-region{margin-left:5%;outline:red solid 1px}.dragster-draggable{position:relative;z-index:1;cursor:pointer}.dragster-draggable+.dragster-draggable{margin-top:1rem}.dragster-draggable.is-dragging{cursor:move}.dragster-temp{position:absolute;z-index:10;top:0;left:0;cursor:move;opacity:1;box-shadow:0 0 5px 0 rgba(0,0,0,.5);transition:opacity .3s ease-in-out,box-shadow .3s ease-in-out}.dragster-block{background:#eee;padding:.5rem;opacity:1}.dragster-draggable.is-dragging>*{opacity:.5}.dragster-drop-placeholder{height:2rem;margin:.5rem 0;border:1px dashed #000}.dragster-is-hidden,.dragster-temp.dragster-is-hidden{opacity:0;box-shadow:none}.dragster-draggable.dragster-replacable{outline:#b30 solid 2px}.error{border:1px solid red;}";

Page myPages[] = {
		{ "/index", "text/html", index_html },
		{ "/scripts/init.js", "application/javascript", init_js },
		{ "/styles/drag.css", "text/css", drag_css },
		{ "/scripts/drag.js", "application/javascript", drag_js },
		{ nullptr }
};
void myPage(const char* url, ResponseCallback* cb, void* cbArg, Reader* body, Writer* result, void* reserved);
STARTUP(softap_set_application_page_handler(myPage, nullptr));

void setup() {

	relayController.setAddress(0,0,0);
	inputBoard.setAddress(1,0,0);
	Serial1.begin(115200);
	getInfoFromMemory();
	//	for(int i = 0; i < 2047; i++){
	//		EEPROM.write(i,255);
	//	}
	//	Serial.println("Done");
}

void loop() {
	//Check Status of inputs
		unsigned long start = millis();
	if(readInputs()){
		updateSlaveStatus();
	}

//	    Serial.printf("Round Trip: %i \n", millis() - start);
	roundTrip = millis() - start;
}

bool readInputs(){
//	Serial.println("Read All Inputs");
	int slaveRelayStatus[8] = {0,0,0,0,0,0,0,0};
	inputBoard.readAllInputs(inputStatus);
	if(inputStatus[0] == 256){
		Serial.println("Input board failed");
		return false;
	}
	byte data[2];
//	Serial.printf("Read board inputs complete, input status: %i \n", inputStatus);

	for(int i = 0; i < numberOfSlaves; i++){
//		Serial.printf("Slave %i transmission start \n", i);
		if(deviceStatus[i] == 1){
			int bitpos = 1 << ((i<4 ? i : i%4) * 2);
			if(~inputStatus[(int)floor(i/4)] & (bitpos)){
				slaveRelayStatus[i] = slaveRelayStatus[i] | 1;
			}
			if(~inputStatus[(int)floor(i/4)] & (bitpos << 1)){
				slaveRelayStatus[i] = slaveRelayStatus[i] | 2;
			}
			data[0] = 0;
			data[1] = slaveRelayStatus[i];
//			Serial.printf("Sending transmission to slave %i \n", i);
			if(!module.transmit(slaveAddress[i], data, 2)){
				Serial.printf("Transmission to slave Slave %i failed. \n", i+1);
				deviceFailCount[i]++;
			}else{
				if(deviceStartTime[i] == 0){
					deviceStartTime[i] = millis();
				}
				deviceRSSI[i] = module.getRSSI();
				deviceSuccessCount[i]++;
//				Serial.printf("Slave %i rssi = %i \n", i, deviceRSSI[i]);
			}
			delay(10);
		}
	}
	return true;
}

void updateLocalRelays(byte* packet, int packetLen){
	//get length of new data

	byte senderAddress[8];

	int receivedDataLength = module.getReceiveDataLength(packet);
	char receivedData[receivedDataLength];
	int validDataCount = module.parseReceive(packet, receivedData, packetLen, senderAddress);
	if(validDataCount == receivedDataLength){

	}
	switch(receivedData[0]){
	case 0:
		int senderID = 9;
		for(int i = 0; i < 8; i++){
			if(memcmp(slaveAddress[i], senderAddress, 8) == 0){
				senderID = i;
				break;
			}
		}
		if(senderID != 9){
			//                Serial.printf("Got update from slave %i \n", senderID+1);
			//                Serial.printf("Update byte is: %i \n", receivedData[1]);
			lastHeard[senderID] = millis();
			int firstRelay = (senderID*2)+1;
			int secondRelay = (senderID*2)+2;
			if(receivedData[1]&1){
				//                	Serial.printf("Turn on relay %i \n", firstRelay);
				relayController.turnOnRelay(firstRelay);
			}else{
				//                	Serial.printf("Turn off relay %i \n", firstRelay);
				relayController.turnOffRelay(firstRelay);
			}
			if(receivedData[1]&2){
				//                	Serial.printf("Turn on relay %i \n", secondRelay);
				relayController.turnOnRelay(secondRelay);
			}else{
				//                	Serial.printf("Turn off relay %i \n", secondRelay);
				relayController.turnOffRelay(secondRelay);
			}
		}else{
			Serial.println("Device Address not registered");
		}
		break;
	}
}

void updateSlaveStatus(){
	for(int i = 0; i < numberOfSlaves; i++){
		if(millis() > slaveTimeout + lastHeard[i]){
			if(deviceStatus[i] != 0){
//				Serial.printf("Slave %i went offline \n", i+1);
				deviceStartTime[i] = 0;
				deviceStatus[i] = 0;
				deviceFailTime[i] = millis();
				relayController.turnOffRelay((i*2)+1);
				relayController.turnOffRelay((i*2)+2);
				deviceRSSI[i] = 256;
			}else{
				if(millis() > deviceFailTime[i] + slaveRetryInterval){
//					Serial.printf("Attempting reconnect to slave %i \n", i+1);
					deviceStatus[i] = 1;
				}
			}

		}
	}
}

void getInfoFromMemory(){
	Serial.println("GetInfoFromMemory()");
	//Get number of slaves from memory
	int memSlaveCount = EEPROM.read(0);
	(memSlaveCount !=255 ? numberOfSlaves = memSlaveCount : numberOfSlaves = 8);
	//Get slave com timeout from memory
	unsigned long memSlaveTimeout;
	EEPROM.get(1, memSlaveTimeout);
	(memSlaveTimeout != 4294967295 ? slaveTimeout = memSlaveTimeout : slaveTimeout = 10000);
	//Get offline slave retry interval from memory
	unsigned long memSlaveRetryInterval;
	EEPROM.get(5, memSlaveRetryInterval);
	(memSlaveRetryInterval != 4294967295 ? slaveRetryInterval = memSlaveRetryInterval : slaveRetryInterval = 20000);
	//Get Slave addresses from memory
	//Currently have a spare byte at the end of each device.  Could be used for slave status possibly
	for(int i = 0; i < numberOfSlaves; i++){
		byte memSlaveAddr[8];
		for(int j = (i*9)+9; j < (i*9)+17; j++){
			memSlaveAddr[j - (i*9)-9] = EEPROM.read(j);
			Serial.printf("j: %i : ", j);

			slaveAddress[i][j - (i*9)-9] = memSlaveAddr[j - (i*9)-9];
			Serial.printf("%i : %02X \n", j - (i*9)-9, slaveAddress[i][j - (i*9)-9]);
		}
		Serial.printf("%02X%02X%02X%02X%02X%02X%02X%02X \n", slaveAddress[i][0],slaveAddress[i][1],slaveAddress[i][2],slaveAddress[i][3],slaveAddress[i][4],
				slaveAddress[i][5],slaveAddress[i][6],slaveAddress[i][7]);
		//print slave address to string used for access from web interface.
		sprintf(stringSlaveAddress[i], "%02X%02X%02X%02X%02X%02X%02X%02X", slaveAddress[i][0],slaveAddress[i][1],slaveAddress[i][2],slaveAddress[i][3],slaveAddress[i][4],
				slaveAddress[i][5],slaveAddress[i][6],slaveAddress[i][7]);
//		Serial.printf("Device %i address: ", i);
//		Serial.println(String(stringSlaveAddress[i]));
	}

}

void myPage(const char* url, ResponseCallback* cb, void* cbArg, Reader* body, Writer* result, void* reserved)
{
	Serial.printlnf("handling page %s", url);
	int8_t idx = 0;
	for (;;idx++) {
		Page& p = myPages[idx];
		if (!p.url) {
			idx = -1;
			break;
		}
		else if (strcmp(url, p.url)==0) {
			break;
		}
	}
	Serial.println(idx);
	if(idx==-1){
		if(strstr(url,"devices.js")){
			Serial.println("devices.js started");

			//System Latency
			String latency = "latency=\"";
			latency.concat(roundTrip);
			latency.concat("\";");

			//Device address
			String devices;
			devices.concat(stringSlaveAddress[0]);
			Serial.println(devices);
			String output="devices=\"";
			output.concat(devices);
			output.concat("\";");

			//Device RSSI
			String rssi="rssi=\"";
			for(int i = 0; i < numberOfSlaves; i++){
				rssi.concat(String(deviceRSSI[i]));
				rssi.concat(",");
			}
			rssi.concat("\";");

			String failSuccess = "failSuccessCount=\"";
			for(int i = 0; i < numberOfSlaves; i++){
				failSuccess.concat(String(deviceFailCount[i]));
				failSuccess.concat(",");
				failSuccess.concat(String(deviceSuccessCount[i]));
				failSuccess.concat("|");
			}
			failSuccess.concat("\";");

			//Device Up Time in hours:minutes:seconds
			String deviceUp = "deviceUpTime=\"";
			for(int i = 0; i < numberOfSlaves; i++){
				unsigned long up = millis() - deviceStartTime[i];
				int hours = up/3600000;
				up = up - (hours *3600000);
				int minutes = up/60000;
				up = up - (minutes*60000);
				int seconds = up/1000;
				deviceUp.concat(String(hours));
				deviceUp.concat(":");
				deviceUp.concat(String(minutes));
				deviceUp.concat(":");
				deviceUp.concat(String(seconds));
				deviceUp.concat(",");
			}
			deviceUp.concat("\";");

			cb(cbArg, 0, 200, "application/javascript", nullptr);
			result->write(latency);
			result->write(output);
			result->write(rssi);
			result->write(failSuccess);
			result->write(deviceUp);
			Serial.println("devices.js finished");
		}
		else{
			if(strstr(url,"index")){
				//This runs on submit
				if(strstr(url,"?")){
					Serial.println("index? started");
					String parsing = String(url);
					String devices;
					byte count = 0;
					int next = parsing.indexOf("=");
					Serial.println(parsing);
					while(next > -1){

						parsing = parsing.substring(next+1);
						int end = parsing.indexOf("&");
						if(end == 0){
							next = parsing.indexOf("=", 0);
							continue;
						}
						device = parsing.substring(0,end);

						Serial.println(device);
						char hexstring[17];
						device.toCharArray(hexstring, 17);

						unsigned int addrByte[8];
						Serial.print("New Address: ");
						for(int i = 0; i < 8; i++){
							sscanf(&hexstring[i*2], "%2x", &addrByte[i]);
							EEPROM.write((count*9)+9+i, addrByte[i]);

							Serial.printf("%02X", addrByte[i]);
						}
						Serial.println();
						devices.concat(device);
						devices.concat("1");
						next = parsing.indexOf("=", 0);
						Serial.print("parsing: ");
						Serial.println(parsing);
						count++;
					}
					EEPROM.write(0, count);
//					if(parsing.length() == 8){
//						Serial.println("aftr loop");
//						devices.concat(parsing.substring(0,8));
//						devices.concat("1");
//						count++;
//					}
					//byte stat = 1;
					//EEPROM.put(0, 1);
					//EEPROM.put(1, devices);
					int c = 0;
					for(int i = 9; i < 81; i++){

						if(c == 8){
							c=0;
							Serial.println();
						}else{
							Serial.printf("%02X", EEPROM.read(i));
							c++;
						}
					}
					getInfoFromMemory();
					System.reset();
					Serial.println("sending redirect");
					Header h("Location: /index\r\n");
					cb(cbArg, 0, 302, "text/plain", &h);
					return;
				}
				else{
					Serial.println("oops, something went wrong on index");
					Serial.println("404 error");
					cb(cbArg, 0, 404, nullptr, nullptr);
				}
			}
			else{
				if(strstr(url,"reset.html")){
					byte num=1;
					EEPROM.write(0,num);
					String devices="";
					EEPROM.put(1,devices);
					Serial.println("sending redirect");
					Header h("Location: /index\r\n");
					cb(cbArg, 0, 302, "text/plain", &h);
					return;
				}
				Serial.println("404 error");
				cb(cbArg, 0, 404, nullptr, nullptr);
			}

		}
	}else{
		Serial.println("Normal page request");
		cb(cbArg, 0, 200, myPages[idx].mime_type, nullptr);
		result->write(myPages[idx].data);
		Serial.println("result written");
	}
	return;
}
