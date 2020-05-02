#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>


static const char ssid[] = "Tech_D0060840"; //"Dom"; //set Your SSID
static const char password[] = "JRRVPVPB"; //"TP-LINK TL-WN422G"; //set Your password
#define reczneIP 0 //for statiC IP set to 1

//and put hire your lan configuration:
#if (reczneIP)
  IPAddress ip(192, 168, 0, 104);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 0, 0);
  IPAddress dns(192, 168, 1, 1);
#endif

#define numberOfElements 9
#define numberOfSmdElements 2

#define PIRPin 16 //D0
#define PhotoRPin A0
#define PhotoRTreshold 300

//type of elements
int typeSmd = 5;
int typeLed = 6;
int typeOther = 7;
int typeAllLed = 8;
int typeAutoMode = 9;
int typeAutoModeSmd = 10;
int typeAllSmd = 11;



static void writePin(bool, int);
static void turnOn(int, int );
static void turnOff(int, int);
static void turn(int, int, bool);
static void autoModeFunction(bool, int);
static void setPwm(int, int, uint8_t *);


bool AutoModeState = 0;
bool AutoModeSmdState = 0;
bool PIRState = 0;
int PhotoRState = 1000;
bool AutoModePwmEnabled = 0;
bool AutoModeSmdPwmEnabled =0;
int AutoModePwmValue = 0;
int AutoModeSmdPwmValue =0;
    
struct elements
{
  const char* name;
  bool state=0;
  const char* onName;
  const char* offName;
  const char* pwnName = "pwn";
  int pin = 555;
  int type;
  int pwmValue = 0;
   
};
typedef struct elements Elements;
Elements tabOfElements[numberOfElements];
Elements tabOfSmdElements[numberOfSmdElements];

void addElements()
{
  tabOfElements[0].name = "RedLeftLed";
  tabOfElements[0].onName = "RedLeftLedOn";
  tabOfElements[0].offName = "RedLeftLedOff";
  tabOfElements[0].pwnName = "RLPWM";
  tabOfElements[0].pin = 5; //D1
  tabOfElements[0].type = typeLed;

  tabOfElements[1].name = "GreenLeftLed";
  tabOfElements[1].onName = "GreenLeftLedOn";
  tabOfElements[1].offName = "GreenLeftLedOff";
  tabOfElements[1].pwnName = "GLPWM";
  tabOfElements[1].pin = 4; //D2
  tabOfElements[1].type = typeLed;
    
  tabOfElements[2].name = "YellowLeftLed";
  tabOfElements[2].onName = "YellowLeftLedOn";
  tabOfElements[2].offName = "YellowLeftLedOff";
  tabOfElements[2].pwnName = "YLPWM";
  tabOfElements[2].pin = 0; //D3 
  tabOfElements[2].type = typeLed;  

  tabOfElements[3].name = "BlueLed";
  tabOfElements[3].onName = "BlueLedOn";
  tabOfElements[3].offName = "BlueLedOff";
  tabOfElements[3].pwnName = "BCPWM";
  tabOfElements[3].pin = 2; //D4 
  tabOfElements[3].type = typeLed; 
 
  tabOfElements[4].name = "GreenRightLed";
  tabOfElements[4].onName = "GreenRightLedOn";
  tabOfElements[4].offName = "GreenRightLedOff";
  tabOfElements[4].pwnName = "GRPWM";
  tabOfElements[4].pin = 14; //D5
  tabOfElements[4].type = typeLed;

  tabOfElements[5].name = "YellowRightLed";
  tabOfElements[5].onName = "YellowRightLedOn";
  tabOfElements[5].offName = "YellowRightLedOff";
  tabOfElements[5].pwnName = "YRPWM";
  tabOfElements[5].pin = 12; //D6
  tabOfElements[5].type = typeLed;

  tabOfElements[6].name = "RedRightLed";
  tabOfElements[6].onName = "RedRightLedOn";
  tabOfElements[6].offName = "RedRightLedOff";
  tabOfElements[6].pwnName = "RRPWM";
  tabOfElements[6].pin = 13; //D7
  tabOfElements[6].type = typeLed;

  tabOfElements[7].name = "All";
  tabOfElements[7].onName = "AllOn";
  tabOfElements[7].offName = "AllOff";
  tabOfElements[7].pwnName = "ALPWM";
  tabOfElements[7].type = typeAllLed;

  tabOfElements[8].name = "AutoMode";
  tabOfElements[8].onName = "AutoModeOn";
  tabOfElements[8].offName = "AutoModeOff";
  tabOfElements[8].pwnName = "AMPWM";
  tabOfElements[8].type = typeAutoMode;

  tabOfSmdElements[0].name = "Smd";
  tabOfSmdElements[0].onName = "SmdOn";
  tabOfSmdElements[0].offName = "SmdOff";
  tabOfSmdElements[0].pwnName = "SDPWM";
  tabOfSmdElements[0].pin = 15; //D8
  tabOfSmdElements[0].type = typeSmd;


  tabOfSmdElements[1].name = "AutoModeSmd";
  tabOfSmdElements[1].onName = "AutoModeSmdOn";
  tabOfSmdElements[1].offName = "AutoModeSmdOff";
  tabOfSmdElements[1].pwnName = "SMPWM";
  tabOfSmdElements[1].type = typeAutoModeSmd;   
}

MDNSResponder mdns;
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);

WebSocketsServer webSocket = WebSocketsServer(81);

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
    <title>Colorful LEDs</title>
    <style>
    "body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }"
    #container
    {
    width: 100%;
    margin-left: auto;
    margin-right: auto;}
    
    #title
    {
      background-color:black;
      color:white;  
      text-align: center;
      padding: 1.5%;
    }
    #text
    {
      
      background-color: #CCFFFF;
      color: black;
      line-height: 120%;
      text-align: center;
      padding: 1.5%;
      margin-top: 1%;
    }
        
    #otherDev
    {
      
      width: 30%;
      height: 200%;
      background-color: #BBBBBB;
      color: grey;
      text-align: left;
      padding: 10px;
    }
    #con
    {
      
      background-color: #AAAAAA;
           color: blue;
      text-align: left;
      padding: 10px;
    }
    .slidecontainer {
      width: 100%; /* Width of the outside container */
      height: 30px;
      margin-top: 5px;
    }
    
    /* The slider itself */
    .slider {
      -webkit-appearance: none;  /* Override default CSS styles */
      appearance: none;
      width: 100%; 
      height: 15px; /* Specified height */
      background: #d3d3d3; /* Grey background */
      outline: none; /* Remove outline */
      opacity: 0.7; /* Set transparency (for mouse-over effects on hover) */
      -webkit-transition: .2s; /* 0.2 seconds transition on hover */
      transition: opacity .2s;
    }
    
    /* Mouse-over effects */
    .slider:hover {
      opacity: 1; /* Fully shown on mouse-over */
    }
    
    /* The slider handle (use -webkit- (Chrome, Opera, Safari, Edge) and -moz- (Firefox) to override default look) */
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none; /* Override default look */
      appearance: none;
      width: 25px; /* Set a specific slider handle width */
      height: 25px; /* Slider handle height */
      border-radius: 50%;
      background: #000000; /* Green background */
      cursor: pointer; /* Cursor on hover */
    }
    
    .slider::-moz-range-thumb {
      width: 25px; /* Set a specific slider handle width */
      height: 25px; /* Slider handle height */
      border-radius: 50%;
      background: #000000; /* Green background */
      cursor: pointer; /* Cursor on hover */
    }


    </style>
    <script>
    var websock;
    function start() {
    websock = new WebSocket('ws://' + window.location.hostname + ':81/');
      websock.onopen = function(evt) { console.log('websock open'); };
      websock.onclose = function(evt) { console.log('websock close'); };
      websock.onerror = function(evt) { console.log(evt); };
      
      websock.onmessage = function(evt) {
        console.log(evt);
      var str = evt.data;
        var res = str.split("@&");
         console.log(res[0]);
       if (res [0]=== 'start')
       {
        console.log('rozpoznal');
        document.getElementById("txt1").value = res[1];
        document.getElementById("txt2").value = res[2];
        document.getElementById("txt3").value = res[3];
        document.getElementById("txt4").value = res[4];
       }
        var rl = document.getElementById('RedLeftLedStatus');
        var yl = document.getElementById('YellowLeftLedStatus');
        var gl = document.getElementById('GreenLeftLedStatus');
        var b = document.getElementById('BlueLedStatus');
        var rr = document.getElementById('RedRightLedStatus');
        var yr = document.getElementById('YellowRightLedStatus');
        var gr = document.getElementById('GreenRightLedStatus');
        var all = document.getElementById('AllStatus'); 
        var autoM = document.getElementById('AutoModeStatus');
        var smd = document.getElementById('SmdStatus');
        var asmd = document.getElementById('AutoModeSmdStatus');
       
        if (evt.data === 'RedLeftLedOn') {
                    
          rl.style.color = 'red';
        }
        if (evt.data === 'RedLeftLedOff') {                  
          rl.style.color = 'black';
        }
        if (evt.data === 'YellowLeftLedOn') {         
          yl.style.color = 'yellow';
        }
        if (evt.data === 'YellowLeftLedOff') {                 
          yl.style.color = 'black';
        }
        if (evt.data === 'GreenLeftLedOn') {                  
          gl.style.color = 'green';
        }
        if (evt.data === 'GreenLeftLedOff') {                  
          gl.style.color = 'black';
        }
        if (evt.data === 'BlueLedOn') {                  
          b.style.color = 'blue';
        }
        if (evt.data === 'BlueLedOff') {                 
          b.style.color = 'black';
        }

        if (evt.data === 'RedRightLedOn') {
                    
          rr.style.color = 'red';
        }
        if (evt.data === 'RedRightLedOff') {                  
          rr.style.color = 'black';
        }
        if (evt.data === 'YellowRightLedOn') {         
          yr.style.color = 'yellow';
        }
        if (evt.data === 'YellowRightLedOff') {                 
          yr.style.color = 'black';
        }
        if (evt.data === 'GreenRightLedOn') {                  
          gr.style.color = 'green';
        }
        if (evt.data === 'GreenRightLedOff') {                  
          gr.style.color = 'black';
        }
        if (evt.data === 'AllOn') {                  
          all.style.color = 'red';
        }
        if (evt.data === 'AllOff') {                  
          all.style.color = 'black';
        }
        if (evt.data === 'AutoModeOn') {                  
          autoM.style.color = 'red';
        }
        if (evt.data === 'AutoModeOff') {                  
          autoM.style.color = 'black';
        }
        
        if (evt.data === 'SmdOn') {                  
          smd.style.color = 'silver';
        }
        if (evt.data === 'SmdOff') {                  
          smd.style.color = 'black';
        }
        if (evt.data === 'AutoModeSmdOn') {                  
          asmd.style.color = 'silver';
        }
        if (evt.data === 'AutoModeSmdOff') {                  
          asmd.style.color = 'black';
        }        

      };
    }
    
    function buttonclick(e) 
    {
      websock.send(e.id);
    }

   function sliderchange(e) 
   {
      websock.send(e.id +e.value);
   }

 
    </script>
  </head>
<body onload="javascript:start();">
  <div id="container">
  <div id="title">
    <h1>Colorful LEDs</h1>
  </div>
  
  <div id="text">
    <h2>SMD LEDs</h2>
    <br>
  
    <div id="AutoModeSmdStatus"><b>Automation mode</b> </div>
    <div> <button id="AutoModeSmdOn" type="button" onclick="buttonclick(this);" px>On</button> <button id="AutoModeSmdOff" type="button" onclick="buttonclick(this);">Off</button></div>
    <div class="slidecontainer">
    <input type="range" min="0" max="1023" value="512" class="slider" id="SMPWM" onchange="sliderchange(this);" >
    </div>    
    <br>
    
    <div id="SmdStatus"><b>SMD</b> </div>
    <div> <button id="SmdOn" type="button" onclick="buttonclick(this);" px>On</button> <button id="SmdOff" type="button" onclick="buttonclick(this);">Off</button></div>
    <div class="slidecontainer">
    <input type="range" min="0" max="1023" value="512" class="slider" id="SDPWM" onchange="sliderchange(this);" >
    </div>      
    <br>
    
  </div>
  <div id="text">
    <h2>Colorful LEDs</h2>
    <br>
    
    <div id="AutoModeStatus"><b>Automation mode</b> </div>
    <div> <button id="AutoModeOn" type="button" onclick="buttonclick(this);" px>On</button> <button id="AutoModeOff" type="button" onclick="buttonclick(this);">Off</button></div>
    <div class="slidecontainer">
    <input type="range" min="0" max="1023" value="512" class="slider" id="AMPWM" onchange="sliderchange(this);" >
    </div>    
    <br>
    
    <div id="AllStatus"><b>All LEDs</b> </div>
    <div> <button id="AllOn" type="button" onclick="buttonclick(this);" px>On</button> <button id="AllOff" type="button" onclick="buttonclick(this);">Off</button></div>
    <div class="slidecontainer">
    <input type="range" min="0" max="1023" value="512" class="slider" id="ALPWM" onchange="sliderchange(this);" >
    </div>      
    <br>
    
    <div id="RedLeftLedStatus"><b>Red left</b> </div>
    <div> <button id="RedLeftLedOn" type="button" onclick="buttonclick(this);" px>On</button> <button id="RedLeftLedOff" type="button" onclick="buttonclick(this);">Off</button></div>
    <div class="slidecontainer">
    <input type="range" min="0" max="1023" value="512" class="slider" id="RLPWM" onchange="sliderchange(this);" >
    </div>      
    <br>
    
    <div id="YellowLeftLedStatus"><b>Yellow left</b> </div>
    <div> <button id="YellowLeftLedOn" type="button" onclick="buttonclick(this);">On</button> <button id="YellowLeftLedOff" type="button" onclick="buttonclick(this);">Off</button></div>
    <div class="slidecontainer">
    <input type="range" min="0" max="1023" value="512" class="slider" id="YLPWM" onchange="sliderchange(this);" >
    </div>      
    <br>
    
    <div id="GreenLeftLedStatus"><b>Green left</b> </div>
    <div> <button id="GreenLeftLedOn" type="button" onclick="buttonclick(this);">On</button> <button id="GreenLeftLedOff" type="button" onclick="buttonclick(this);">Off</button></div>
    <div class="slidecontainer">
    <input type="range" min="0" max="1023" value="512" class="slider" id="GLPWM" onchange="sliderchange(this);" >
    </div>      
    <br>
    
    <div id="BlueLedStatus"><b>Blue</b> </div>
    <div> <button id="BlueLedOn" type="button" onclick="buttonclick(this);">On</button> <button id="BlueLedOff" type="button" onclick="buttonclick(this);">Off</button></div>
    <div class="slidecontainer">
    <input type="range" min="0" max="1023" value="512" class="slider" id="BCPWM" onchange="sliderchange(this);" >
    </div>      
    <br>
    
    <div id="GreenRightLedStatus"><b>Green left</b> </div>
    <div> <button id="GreenRightLedOn" type="button" onclick="buttonclick(this);">On</button> <button id="GreenRightLedOff" type="button" onclick="buttonclick(this);">Off</button></div>
    <div class="slidecontainer">
    <input type="range" min="0" max="1023" value="512" class="slider" id="GRPWM" onchange="sliderchange(this);" >
    </div>      
    <br>
    
    <div id="YellowRightLedStatus"><b>Yellow left</b> </div>
    <div> <button id="YellowRightLedOn" type="button" onclick="buttonclick(this);">On</button> <button id="YellowRightLedOff" type="button" onclick="buttonclick(this);">Off</button></div>
    <div class="slidecontainer">
    <input type="range" min="0" max="1023" value="512" class="slider" id="YRPWM" onchange="sliderchange(this);" >
    </div>      
    <br>
    
    <div id="RedRightLedStatus"><b>Red left</b> </div>
    <div> <button id="RedRightLedOn" type="button" onclick="buttonclick(this);" width=100px>On</button> <button id="RedRightLedOff" type="button" onclick="buttonclick(this);">Off</button></div>
    <div class="slidecontainer">
    <input type="range" min="0" max="1023" value="512" class="slider" id="RRPWM" onchange="sliderchange(this);" >
    </div>    
    <br>
  </div>

  <div id="otherDev">
    <b>Next Device</b>
  </div>
  <div id="con">
    <div id="con">
    <div id="txta1"><b>T1=</b> <INPUT TYPE="text" id="txt1" NAME="text1" VALUE="N/A   " SIZE=5>&deg C</div>
    <div id="txta2"><b>T2=</b> <INPUT TYPE="text" id="txt2" NAME="text2" VALUE="N/A   " SIZE=5>&deg C</div>
    <div id="txta3"><b>T3=</b> <INPUT TYPE="text" id="txt3" NAME="text3" VALUE="N/A   " SIZE=5>&deg C</div>
    <div id="txta4"><b>T4=</b> <INPUT TYPE="text" id="txt4" NAME="text4" VALUE="N/A   " SIZE=5>&deg C</div>
    <div id="tex5">............................... </div>
  </div>
    
 
</div>
</body>
</html>
)rawliteral";

String webString = "", token="@&"; 

void refreshWebSocket()
{
  
  for (int i=0; i<numberOfElements; i++)
  {
    if(tabOfElements[i].state)
    {
      webSocket.broadcastTXT(tabOfElements[i].onName, strlen(tabOfElements[i].onName));
    }
    else
    {
      webSocket.broadcastTXT(tabOfElements[i].offName, strlen(tabOfElements[i].offName));
    }
  }
  for (int i=0; i<numberOfSmdElements; i++)
  {
    if(tabOfSmdElements[i].state)
    {
      webSocket.broadcastTXT(tabOfSmdElements[i].onName, strlen(tabOfSmdElements[i].onName));
    }
    else
    {
      webSocket.broadcastTXT(tabOfSmdElements[i].offName, strlen(tabOfSmdElements[i].offName));
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{

  
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        for (int i=0; i<numberOfElements; i++)
        {
          if (tabOfElements[i].state)
          {
            webSocket.sendTXT(num, tabOfElements[i].onName, strlen(tabOfElements[i].onName));  
          }
          else
          {
            webSocket.sendTXT(num, tabOfElements[i].offName, strlen(tabOfElements[i].offName));              
          }
        }
        webSocket.sendTXT(num,webString);
      }
      break;
    case WStype_TEXT:

      Serial.printf("[%u] get Text: %s\r\n", num, payload);

      for (int i=0; i<numberOfElements; i++)
      {
        if (!AutoModeState)
        {
          if (strcmp(tabOfElements[i].onName, (const char *)payload) == 0)
          {
            tabOfElements[i].state = 1;
            turnOn(tabOfElements[i].type, tabOfElements[i].pin);
          }
          else if (strcmp(tabOfElements[i].offName, (const char *)payload) == 0)
          {
            tabOfElements[i].state = 0;
            turnOff(tabOfElements[i].type, tabOfElements[i].pin);
          }
          else if (strncmp(tabOfElements[i].pwnName, (const char *)payload, 5) == 0)
          {
            uint8_t* x=0;
            int y=0;
            x=(payload+5);  
            const char* str = (char*) x;
            y=atoi(str);
            tabOfElements[i].pwmValue = atoi(str);
            if ((tabOfElements[i].name == "AutoMode") && (AutoModeState ==0))
            {
              
            }
            else
            {
            tabOfElements[i].state = 1;  
            }
            
            setPwm(tabOfElements[i].type, tabOfElements[i].pin, tabOfElements[i].pwmValue);
          }
          
          else 
          {
            Serial.println("Unknown command");
          }    

          //webSocket.broadcastTXT(payload, length);   
        }   
        else 
        {
          if (!strcmp(tabOfElements[i].name, "AutoMode"))
          {
            if (strcmp(tabOfElements[i].offName, (const char *)payload) == 0) 
            {
              tabOfElements[i].state=0;
              AutoModeState=0;
              AutoModePwmEnabled = 0;
              turnOff(typeAllLed, 555);
              webSocket.broadcastTXT(payload, length);              
            }
            if (strncmp(tabOfElements[i].pwnName, (const char *)payload, 5) == 0)
            {
              uint8_t* x=0;
              int y=0;
              x=(payload+5);  
              const char* str = (char*) x;
              y=atoi(str);
              tabOfElements[i].pwmValue = atoi(str);
              AutoModePwmValue = tabOfElements[i].pwmValue;
              setPwm(tabOfElements[i].type, tabOfElements[i].pin, tabOfElements[i].pwmValue);
            }
          }          
        }
      }
      
      for (int i=0; i<numberOfSmdElements; i++)
      {
        if (!AutoModeSmdState)
        {
          if (strcmp(tabOfSmdElements[i].onName, (const char *)payload) == 0)
          {
            tabOfSmdElements[i].state = 1;
            turnOn(tabOfSmdElements[i].type, tabOfSmdElements[i].pin);
          }
          else if (strcmp(tabOfSmdElements[i].offName, (const char *)payload) == 0)
          {
            tabOfSmdElements[i].state = 0;
            turnOff(tabOfSmdElements[i].type, tabOfSmdElements[i].pin);
          }
          else if (strncmp(tabOfSmdElements[i].pwnName, (const char *)payload, 5) == 0)
          {
            uint8_t* x=0;
            int y=0;
            x=(payload+5);  
            const char* str = (char*) x;
            y=atoi(str);
            tabOfSmdElements[i].pwmValue = atoi(str);
            if ((tabOfSmdElements[i].name == "AutoModeSmd") && (AutoModeSmdState ==0))
            {
              
            }
            else
            {
            tabOfSmdElements[i].state = 1;  
            }
            
            setPwm(tabOfSmdElements[i].type, tabOfSmdElements[i].pin, tabOfSmdElements[i].pwmValue);
          }
          
          else 
          {
            Serial.println("Unknown command");
          }    

          //webSocket.broadcastTXT(payload, length);   
        }   
        else 
        {
          if (!strcmp(tabOfSmdElements[i].name, "AutoModeSmd"))
          {
            if (strcmp(tabOfSmdElements[i].offName, (const char *)payload) == 0) 
            {
              tabOfSmdElements[i].state=0;
              AutoModeSmdState=0;
              AutoModeSmdPwmEnabled = 0;
              turnOff(typeAllSmd, 555);
              webSocket.broadcastTXT(payload, length);              
            }
            if (strncmp(tabOfSmdElements[i].pwnName, (const char *)payload, 5) == 0)
            {
              uint8_t* x=0;
              int y=0;
              x=(payload+5);  
              const char* str = (char*) x;
              y=atoi(str);
              tabOfSmdElements[i].pwmValue = atoi(str);
              AutoModeSmdPwmValue = tabOfSmdElements[i].pwmValue;
              setPwm(tabOfSmdElements[i].type, tabOfSmdElements[i].pin, tabOfSmdElements[i].pwmValue);
            }
          }          
        }
        
      }
      break;
  
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      hexdump(payload, length);

      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
      
    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
 
  }
}

void handleRoot()
{
  server.send(200, "text/html", INDEX_HTML);
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

/*____________________________________________PIN FUNCTION________________________________________________*/

static void setPwm(int Type, int Pin, int Value)
{
  if ((Type == typeLed) || (Type == typeSmd))
  {
    analogWrite(Pin, Value);
  }
  else if  (Type == typeAllLed)
  {
    for (int i=0; i<numberOfElements; i++)
    {
      if(!strcmp(tabOfElements[i].name, "All"))
      {
         tabOfElements[i].state = 1;
      }
      if (tabOfElements[i].type == typeLed)
      {
        tabOfElements[i].state = 1;
        if (AutoModeState == 1)
        {
          analogWrite(tabOfElements[i].pin, AutoModePwmValue);
        }
        
        else
        {
          analogWrite(tabOfElements[i].pin, Value);
        }
      }
    }       
  }
else if  (Type == typeAllSmd)
  {
    for (int i=0; i<numberOfSmdElements; i++)
    {
      if(!strcmp(tabOfSmdElements[i].name, "AllSmd"))
      {
         tabOfSmdElements[i].state = 1;
      }
      if (tabOfSmdElements[i].type == typeSmd)
      {
        tabOfSmdElements[i].state = 1;
        if (AutoModeSmdState == 1)
        {
          analogWrite(tabOfSmdElements[i].pin, AutoModeSmdPwmValue);
        }
        
        else
        {
          analogWrite(tabOfSmdElements[i].pin, Value);
        }
      }
    }       
  }  
  if ((Type == typeAutoMode) && (AutoModeState == 1))
  {
    AutoModePwmEnabled = 1;
    setPwm(typeAllLed, 555, AutoModePwmValue);
  }
  if ((Type == typeAutoModeSmd) && (AutoModeSmdState == 1))
  {
    AutoModeSmdPwmEnabled = 1;
    setPwm(typeAllSmd, 555, AutoModeSmdPwmValue);
  }  

  refreshWebSocket();
}


static void turnOn(int Type, int Pin)
{
  turn(Type, Pin, 1);
}

static void turnOff(int Type, int Pin)
{
  if (Type == typeLed)
  {
    for (int i=0; i<numberOfElements; i++)
    {
      if(tabOfElements[i].name== "All")
      {
         tabOfElements[i].state = 0;
      }
    }
  }
  turn(Type, Pin, 0);
}

static void turn(int Type, int Pin, bool PinState)
{
  if ((Type == typeLed) || (Type == typeSmd))
  {
    writePin(PinState, Pin);
  }
  else if  (Type == typeAllLed)
  {
    for (int i=0; i<numberOfElements; i++)
    {
      if(!strcmp(tabOfElements[i].name, "All"))
      {
         tabOfElements[i].state = PinState;
      }
      
      if (tabOfElements[i].type == typeLed)
      {
        tabOfElements[i].state = PinState;
        writePin(tabOfElements[i].state, tabOfElements[i].pin);
      }
    }
  }  
  else if  (Type == typeAllSmd)
  {
    for (int i=0; i<numberOfSmdElements; i++)
    {
      if(!strcmp(tabOfSmdElements[i].name, "AllSmd"))
      {
         tabOfSmdElements[i].state = PinState;
      }
      
      if (tabOfSmdElements[i].type == typeSmd)
      {
        tabOfSmdElements[i].state = PinState;
        writePin(tabOfSmdElements[i].state, tabOfSmdElements[i].pin);
      }
    }          
  }
  else if ((Type == typeAutoMode) && (PinState == 1))
  {
    for (int i=0; i<numberOfElements; i++)
    {
      if(!strcmp(tabOfElements[i].name, "AutoMode"))
      {
         tabOfElements[i].state = PinState;
      }
    }
    AutoModeState = 1;
    AutoModePwmEnabled = 0;
    turnOff(typeAllLed, 555);
  }
  else if ((Type == typeAutoModeSmd) && (PinState == 1))
  {
    for (int i=0; i<numberOfSmdElements; i++)
    {
      if(!strcmp(tabOfSmdElements[i].name, "AutoModeSmd"))
      {
         tabOfSmdElements[i].state = PinState;
      }
    }
    AutoModeSmdState = 1;
    AutoModeSmdPwmEnabled = 0;
    turnOff(typeAllSmd, 555);
  }  
  else
  {
    Serial.println("turn else ERROR");
  }
  refreshWebSocket();
}

static void writePin(bool PinState, int Pin)
{
  digitalWrite(Pin, PinState);
}


static void autoModeFunction(bool PIRState, int PhotoRState)
{
  if (AutoModeState)
  {
    if ((PIRState == 1) && (PhotoRState <= PhotoRTreshold))
    {
      if(AutoModePwmEnabled)
      {
        setPwm(typeAutoMode, 555, 555);
      }
      else 
      {
        turnOn(typeAllLed, 555); 
      }
      
    }
    else if ((PIRState == 0) || (PhotoRState >= PhotoRTreshold + 200))
    {
      turnOff(typeAllLed, 555);
    }
  }
  if (AutoModeSmdState)
  {
    if ((PIRState == 1) && (PhotoRState <= PhotoRTreshold))
    {
      if(AutoModeSmdPwmEnabled)
      {
        setPwm(typeAutoModeSmd, 555, 555);
      }
      else 
      {
        turnOn(typeAllSmd, 555); 
      }
      
    }
    else if ((PIRState == 0) || (PhotoRState >= PhotoRTreshold + 200))
    {
      turnOff(typeAllSmd, 555);
    }
  }
}


void setup()
{
  addElements();
  for (int i=0; i<numberOfElements; i++)
  {
    if (tabOfElements[i].type == typeLed)
    {
      pinMode(tabOfElements[i].pin, OUTPUT);
      digitalWrite(tabOfElements[i].pin, LOW); 
    }  
  }
  for (int i=0; i<numberOfSmdElements; i++)
  {
    if (tabOfSmdElements[i].type == typeSmd)
    {
      pinMode(tabOfSmdElements[i].pin, OUTPUT);
      digitalWrite(tabOfSmdElements[i].pin, LOW); 
    }  
  }
  
  Serial.begin(115200);
  #if (reczneIP) 
    Serial.println("reczne IP");
    Serial.println(ip);
    WiFi.config(ip, dns, gateway, subnet);  
  #endif

  Serial.println();
  Serial.println();
  Serial.println();

  for(uint8_t t = 4; t > 0; t--) 
  {
    Serial.printf("[SETUP] BOOT WAIT %d...\r\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFiMulti.addAP(ssid, password);

  while(WiFiMulti.run() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(100);
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());



  if (MDNS.begin("espwebsock", WiFi.localIP())) 
  {
    Serial.println("MDNS responder started");
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
    
  }
  else 
  {
    Serial.println("MDNS.begin failed");
  }
  
  Serial.print("Connect to http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  pinMode(PIRPin, INPUT);
  pinMode(PhotoRPin, INPUT);
  analogWriteFreq(1);
}
/*
int VAR1;
int VAR2;
int VAR3;
int VAR4;
8*/
void loop()
{ 
  
  if (AutoModeState || AutoModeSmdState)
  {
    PIRState = digitalRead(PIRPin);
    PhotoRState = analogRead(PhotoRPin);
    autoModeFunction(PIRState, PhotoRState);      
    delay(200);
  }
  /*
  VAR1=PIRState;
  VAR2=PhotoRState;
  VAR3+=1;
  VAR4+=1;
  webSocket.broadcastTXT("start"+token+String(VAR1)+token+String(VAR2)+token+String(VAR3)+token+String(VAR4)); 
  */
  webSocket.loop();
  server.handleClient();
}
