#include "WS_WIFI.h"

// The name and password of the WiFi access point
const char *ssid = APSSID;                
const char *password = APPSK;               
IPAddress apIP(192, 168, 4, 1);    // Set the IP address of the AP

char ipStr[16];
WebServer server(80);                          

void handleRoot() {
  String myhtmlPage =
    String("") +
    "<html>"+
    "<head>"+
    "    <meta charset=\"utf-8\">"+
    "    <title>ESP32-S3-RS485-WLED</title>"+
    "    <style>" +
    "        body {" +
    "            font-family: Arial, sans-serif;" +
    "            background-color: #f0f0f0;" +
    "            margin: 0;" +
    "            padding: 0;" +
    "        }" +
    "        .header {" +
    "            text-align: center;" +
    "            padding: 20px 0;" +
    "            background-color: #333;" +
    "            color: #fff;" +
    "            margin-bottom: 20px;" +
    "        }" +
    "        .container {" +
    "            max-width: 600px;" +
    "            margin: 10 auto;" +
    "            padding: 20px;" +
    "            background-color: #fff;" +
    "            border-radius: 5px;" +
    "            box-shadow: 0 0 5px rgba(0, 0, 0, 0.3);" +
    "        }" +
    
    "        .input-container {//" +
    "            display: flex;" +
    "            align-items: center;" +
    "            margin-bottom: 15px;" +
    "        }" +
    "        .input-container label {" +
    "            width: 80px;" + 
    "            margin-right: 10px;" +
    "        }" +
    "        .input-container input[type=\"text\"] {" +
    "            flex: 1;" +
    "            padding: 5px;" +
    "            border: 1px solid #ccc;" +
    "            border-radius: 3px;" +
    "            margin-right: 10px; "+ 
    "        }" +
    "        .input-container button {" +
    "            padding: 5px 10px;" +
    "            background-color: #333;" +
    "            color: #fff;" +
    "            font-size: 14px;" +
    "            font-weight: bold;" +
    "            border: none;" +
    "            border-radius: 3px;" +
    "            text-transform: uppercase;" +
    "            cursor: pointer;" +
    "        }" +
    "        .button-container {" +
    "            margin-top: 20px;" +
    "            text-align: center;" +
    "        }" +
    "        .button-container button {" +
    "            margin: 0 5px;" +
    "            padding: 10px 15px;" +
    "            background-color: #333;" +
    "            color: #fff;" +
    "            font-size: 14px;" +
    "            font-weight: bold;" +
    "            border: none;" +
    "            border-radius: 3px;" +
    "            text-transform: uppercase;" +
    "            cursor: pointer;" +
    "        }" +
    "        .button-container button:hover {" +
    "            background-color: #555;" +
    "        }" +
    "        .form-group label {" + 
    "            display: block;" + 
    "            font-weight: bold;" + 
    "        }" + 
    "        .light-row {" +
    "            display: flex;" +
    "            align-items: center;" +
    "            gap: 8px;" +
    "            margin: 8px 0;" +
    "        }" +
    "        .light-row span {" +
    "            min-width: 32px;" +
    "            text-align: right;" +
    "        }" +
    "        .light-row input[type=\"range\"] {" +
    "            flex: 1;" +
    "        }" +
    "        nav {" +
    "            margin: 15px 0;" +
    "            text-align: center;" +
    "        }" +
    "        nav a {" +
    "            padding: 10px 50px;" +
    "            background-color: #333;" +
    "            color: white;" +
    "            text-decoration: none;" +
    "            font-weight: bold;" +
    "            border-radius: 5px;" +
    "        }" +
    "        nav a.relayControlActive {" + 
    "            background-color: #fff;" +   
    "            color: #333;" +
    "            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.3), 0 1px 3px rgba(0, 0, 0, 0.1);" +
    "            transform: translateY(-4px);" +
    "            transition: all 0.2s ease-in-out;" +
    "        }" + 
    "    </style>" +
    "</head>"+
    "<body>"+
    "    <script defer=\"defer\">"+
    "        function SetRS485BaudRate() {"+
    "            var dataType = document.getElementById('RS485BaudRate').value;" +
    "            var WebData = " +
    "                'RS485 BaudRate: ' + dataType + '  ' + '\\n' + " + 
    "                'Web End' + '\\n' ;" + 
    "            var xhr = new XMLHttpRequest();" +
    "            xhr.open('GET', '/RS485SetBaudRate?data=' + WebData, true);" +
    "            xhr.send();" +
    "        }" +
    "        function RS485Config() {"+
    "            var dataType = document.getElementById('RS485ReadDataType').value;" +
    "            var WebData = " +
    "                'Data Type: ' + dataType + '  ' + '\\n' + " + 
    "                'Web End' + '\\n' ;" + 
    "            var xhr = new XMLHttpRequest();" +
    "            xhr.open('GET', '/RS485SetConfig?data=' + WebData, true);" +
    "            xhr.send();" +
    "        }" +
    "        function RS485Send() {"+
    "            var dataType = document.getElementById('DataType').value;" +
    "            var rs485Data = document.getElementById('RS485SendData').value;" +
    "            var WebData = " +
    "                'Data Type: ' + dataType + '  ' + '\\n' + " + 
    "                'RS485 Data: ' + rs485Data + '  ' + '\\n' + " + 
    "                'Web End' + '\\n' ;" + 
    "            var xhr = new XMLHttpRequest();" +
    "            xhr.open('GET', '/RS485Send?data=' + WebData, true);" +
    "            xhr.send();" +
    "        }" +
    "        function handleRS485Input(input) {"+
    "            const dataType = document.getElementById(\"DataType\").value;"+
    "            if (dataType === \"1\") {"+
    "                let raw = input.value.replace(/[^0-9a-fA-F]/g, '');"+
    "                let spaced = raw.match(/.{1,2}/g);"+
    "                input.value = spaced ? spaced.join(' ') : '';"+
    "            }"+
    "        }"+
    "        function RS485Read() {"+
    "            var xhr = new XMLHttpRequest();"+
    "            xhr.open('GET', '/getRS485Data', true);"+
    "            xhr.onreadystatechange = function() {"+
    "              if (xhr.readyState === 4 && xhr.status === 200) {"+
    "                var dataArray = JSON.parse(xhr.responseText);"+
    "                if (dataArray.length > 0 && dataArray[0] !== '') {" +
    "                  var textarea = document.getElementById('RS485ReadData');"+
    "                  var isAtBottom = (textarea.scrollHeight - textarea.scrollTop - textarea.clientHeight) < 10;"+
    "                  textarea.value += dataArray;"+
    "                  if (isAtBottom) {"+
    "                      textarea.scrollTop = textarea.scrollHeight;"+
    "                  }"+
    // "                  var currentData = document.getElementById('RS485ReadData').value;"+
    // "                  document.getElementById('RS485ReadData').value = currentData + dataArray;"+
    "                }"+
    "              }"+
    "            };"+
    "            xhr.send();"+
    "        }"+
    "        function ReadConfig() {"+
    "            var xhr = new XMLHttpRequest();"+
    "            xhr.open('GET', '/getRateConfig', true);"+
    "            xhr.onreadystatechange = function() {"+
    "              if (xhr.readyState === 4 && xhr.status === 200) {"+
    "                var dataArray = JSON.parse(xhr.responseText);"+
    "                if (dataArray.rs485_baud !== undefined) {" +
    "                  document.getElementById('RS485BaudRate').value = dataArray.rs485_baud"+
    "                }"+
    "              }"+
    "            };"+
    "            xhr.send();"+
    "        }"+
    "        function AudioStatusText(text) {"+
    "            var el = document.getElementById('AudioStatus');"+
    "            if (el) el.innerText = text;"+
    "        }"+
    "        function AudioRequest(path) {"+
    "            var xhr = new XMLHttpRequest();"+
    "            xhr.open('GET', path, true);"+
    "            xhr.onreadystatechange = function() {"+
    "              if (xhr.readyState === 4) {"+
    "                if (xhr.status === 200) {"+
    "                  try {"+
    "                    var data = JSON.parse(xhr.responseText);"+
    "                    AudioStatusText(data.status || 'OK');"+
    "                  } catch (e) {"+
    "                    AudioStatusText(xhr.responseText || 'OK');"+
    "                  }"+
    "                } else {"+
    "                  AudioStatusText('Request failed');"+
    "                }"+
    "              }"+
    "            };"+
    "            xhr.send();"+
    "        }"+
    "        function AudioPlayMusic() {"+
    "            AudioStatusText('Play/Pause music...');"+
    "            AudioRequest('/AudioPlayMusic');"+
    "        }"+
    "        function AudioRecord5s() {"+
    "            AudioStatusText('Recording 5s, then playback...');"+
    "            AudioRequest('/AudioRecord5s');"+
    "        }"+
    "        function AudioRefreshStatus() {"+
    "            AudioRequest('/AudioStatus');"+
    "        }"+
    "        function LightStatusText(text) {"+
    "            var el = document.getElementById('LightStatus');"+
    "            if (el) el.innerText = text;"+
    "        }"+
    "        function LightUpdateLabels() {"+
    "            ['R','G','B'].forEach(function(name) {"+
    "                var input = document.getElementById('Light' + name);"+
    "                var label = document.getElementById('Light' + name + 'Value');"+
    "                if (input && label) label.innerText = input.value;"+
    "            });"+
    "        }"+
    "        function LightRequest(path) {"+
    "            var xhr = new XMLHttpRequest();"+
    "            xhr.open('GET', path, true);"+
    "            xhr.onreadystatechange = function() {"+
    "              if (xhr.readyState === 4) {"+
    "                if (xhr.status === 200) {"+
    "                  try {"+
    "                    var data = JSON.parse(xhr.responseText);"+
    "                    LightStatusText(data.status || 'OK');"+
    "                  } catch (e) {"+
    "                    LightStatusText(xhr.responseText || 'OK');"+
    "                  }"+
    "                } else {"+
    "                  LightStatusText('Request failed');"+
    "                }"+
    "              }"+
    "            };"+
    "            xhr.send();"+
    "        }"+
    "        function LightSetConfig() {"+
    "            var gpio = document.getElementById('LightGPIO').value;"+
    "            var count = document.getElementById('LightCount').value;"+
    "            LightRequest('/LightSetConfig?gpio=' + gpio + '&count=' + count);"+
    "        }"+
    "        function LightSetColor() {"+
    "            LightUpdateLabels();"+
    "            var r = document.getElementById('LightR').value;"+
    "            var g = document.getElementById('LightG').value;"+
    "            var b = document.getElementById('LightB').value;"+
    "            LightRequest('/LightSetColor?r=' + r + '&g=' + g + '&b=' + b);"+
    "        }"+
    "        function LightOff() {"+
    "            ['R','G','B'].forEach(function(name) {"+
    "                document.getElementById('Light' + name).value = 0;"+
    "            });"+
    "            LightSetColor();"+
    "        }"+
    "        function LightRefreshStatus() {"+
    "            var xhr = new XMLHttpRequest();"+
    "            xhr.open('GET', '/LightStatus', true);"+
    "            xhr.onreadystatechange = function() {"+
    "              if (xhr.readyState === 4 && xhr.status === 200) {"+
    "                var data = JSON.parse(xhr.responseText);"+
    "                document.getElementById('LightGPIO').value = data.gpio;"+
    "                document.getElementById('LightCount').value = data.count;"+
    "                document.getElementById('LightR').value = data.r;"+
    "                document.getElementById('LightG').value = data.g;"+
    "                document.getElementById('LightB').value = data.b;"+
    "                LightUpdateLabels();"+
    "                LightStatusText(data.status || 'OK');"+
    "              }"+
    "            };"+
    "            xhr.send();"+
    "        }"+                           
    "        ReadConfig();"+     
    "        setTimeout(LightRefreshStatus, 200);"+
    "        var refreshInterval = 500;"+                            
    "        setInterval(RS485Read, refreshInterval);"+                      
    "        setInterval(AudioRefreshStatus, 2000);"+
    "    </script>" +
    "    <div class=\"header\">"+
    "        <h1>ESP32-S3-RS485-WLED</h1>"+
    "    </div>"+
    "    <nav>" +
    "        <a href=\"/\" id=\"SerialControlLink\" class=\"SerialControlActive\">Serial Control</a>" +  
    "    </nav>" +
    "    <div class=\"container\">"+
    "        <div class=\"form-group\">" + 
    "            <label for=\"RS485\">RS485:</label>" +
    "            <select id=\"RS485BaudRate\" style=\"width: 120px; text-align: left;\">" + 
    "                <option value=\"110\">110bps</option>" + 
    "                <option value=\"300\">300bps</option>" + 
    "                <option value=\"600\">600bps</option>" + 
    "                <option value=\"1200\">1200bps</option>" + 
    "                <option value=\"2400\">2400bps</option>" + 
    "                <option value=\"4800\">4800bps</option>" + 
    "                <option value=\"9600\">9600bps</option>" + 
    "                <option value=\"14400\">14400bps</option>" + 
    "                <option value=\"19200\">19200bps</option>" + 
    "                <option value=\"38400\">38400bps</option>" + 
    "                <option value=\"56000\">56000bps</option>" + 
    "                <option value=\"57600\">57600bps</option>" + 
    "                <option value=\"115200\">115200bps</option>" + 
    "                <option value=\"128000\">128000bps</option>" + 
    "                <option value=\"230400\">230400bps</option>" + 
    "                <option value=\"460800\">460800bps</option>" + 
    "                <option value=\"500000\">500000bps</option>" + 
    "                <option value=\"512000\">512000bps</option>" + 
    "                <option value=\"600000\">600000bps</option>" + 
    "                <option value=\"750000\">750000bps</option>" + 
    "                <option value=\"921600\">921600bps</option>" + 
    "                <option value=\"1000000\">1000000bps</option>" + 
    "                <option value=\"1500000\">1500000bps</option>" + 
    "                <option value=\"2000000\">2000000bps</option>" + 
    "                <option value=\"3000000\">3000000bps</option>" + 
    "                <option value=\"5000000\">5000000bps</option>" + 
    "            </select>" +   
    "            <button class=\"btn\" id=\"RS485RateConfig\" onclick=\"SetRS485BaudRate()\">Set BaudRate</button>" + 
    "            <label for=\"RS485Send\" style=\"font-weight:normal;\">RS485 Send Data: </label> "+
    "            <select id=\"DataType\" style=\"width: 120px; text-align: left;\">" + 
    "                <option value=\"0\">字符串(char)</option>" + 
    "                <option value=\"1\">十六进制(hex)</option>" + 
    "            </select>" + 
    "            <span><br style=\"margin-bottom: 20px;\"></span>" + 
    "            <input type=\"text\" id=\"RS485SendData\" style=\"width: 500px;\" value=\"12\" oninput=\"handleRS485Input(this)\">" +   
    "            <button class=\"btn\" id=\"SendData\" onclick=\"RS485Send()\">Send Data</button>" +   
    "            <span><br></span>" + 
    "            <span><br></span>" + 
    "            <label for=\"RS485Read\" style=\"font-weight:normal;\">RS485 Receive Data: </label> "+
    "            <select id=\"RS485ReadDataType\" style=\"width: 120px; text-align: left;\">" + 
    "                <option value=\"0\">字符串(char)</option>" + 
    "                <option value=\"1\">十六进制(hex)</option>" + 
    "            </select>" +   
    "            <button class=\"btn\" id=\"RS485ReadDataTypeConfig\" onclick=\"RS485Config()\">Set Config</button>" +
    "            <span><br></span>" +  
    "            <span style=\"font-size: 8px;\">  注意：配置为字符串时，发送端发送数据若存在不可打印字符(0x00~0x1F, 0x7F)将会出现空白数据</span>" + 
    "            <span><br></span>" +  
    "            <span style=\"font-size: 8px;\">  Note: When configured as a string, if the data sent by the sender contains non-printable characters (0x00~0x1F, 0x7F), blank data will appear</span>" +  
    "            <textarea  id=\"RS485ReadData\" style=\"width: 500px; height: 100px; resize: vertical; word-break: break-all;\" placeholder=\"No data was received...\" ></textarea>" + 
    "        </div>" + 
    "        <div class=\"form-group\" style=\"margin-top: 24px;\">" +
    "            <label for=\"AudioPlayMusic\">Audio:</label>" +
    "            <button class=\"btn\" id=\"AudioPlayMusic\" onclick=\"AudioPlayMusic()\">Play / Pause Music</button>" +
    "            <button class=\"btn\" id=\"AudioRecord5s\" onclick=\"AudioRecord5s()\">Record 5s + Play</button>" +
    "            <span id=\"AudioStatus\" style=\"margin-left: 10px; font-size: 12px;\">Audio ready</span>" +
    "        </div>" +
    "        <div class=\"form-group\" style=\"margin-top: 24px;\">" +
    "            <label for=\"LightGPIO\">Light:</label>" +
    "            <span>GPIO </span><input type=\"number\" id=\"LightGPIO\" min=\"0\" max=\"48\" value=\"2\" style=\"width: 70px;\">" +
    "            <span> Count </span><input type=\"number\" id=\"LightCount\" min=\"1\" max=\"2048\" value=\"160\" style=\"width: 90px;\">" +
    "            <button class=\"btn\" id=\"LightConfig\" onclick=\"LightSetConfig()\">Set Light Config</button>" +
    "            <div class=\"light-row\"><label for=\"LightR\" style=\"width: 28px; font-weight:normal;\">R</label><input type=\"range\" id=\"LightR\" min=\"0\" max=\"255\" value=\"0\" oninput=\"LightUpdateLabels()\" onchange=\"LightSetColor()\"><span id=\"LightRValue\">0</span></div>" +
    "            <div class=\"light-row\"><label for=\"LightG\" style=\"width: 28px; font-weight:normal;\">G</label><input type=\"range\" id=\"LightG\" min=\"0\" max=\"255\" value=\"0\" oninput=\"LightUpdateLabels()\" onchange=\"LightSetColor()\"><span id=\"LightGValue\">0</span></div>" +
    "            <div class=\"light-row\"><label for=\"LightB\" style=\"width: 28px; font-weight:normal;\">B</label><input type=\"range\" id=\"LightB\" min=\"0\" max=\"255\" value=\"0\" oninput=\"LightUpdateLabels()\" onchange=\"LightSetColor()\"><span id=\"LightBValue\">0</span></div>" +
    "            <button class=\"btn\" id=\"LightApply\" onclick=\"LightSetColor()\">Apply Color</button>" +
    "            <button class=\"btn\" id=\"LightOff\" onclick=\"LightOff()\">Off</button>" +
    "            <span id=\"LightStatus\" style=\"margin-left: 10px; font-size: 12px;\">Light ready</span>" +
    "        </div>" +
    "    </div>"+
    "</body>"+
    "</html>";
    
  server.send(200, "text/html", myhtmlPage); 
  printf("The user visited the home page\r\n");
  
}

String escapeJson(const char* input) {
  String output = "";
  while (*input) {
    char c = *input++;
    switch (c) {
      case '\"': output += "\\\""; break;
      case '\\': output += "\\\\"; break;
      case '\b': output += "\\b"; break;
      case '\f': output += "\\f"; break;
      case '\n': output += "\\n"; break;
      case '\r': output += "\\r"; break;
      case '\t': output += "\\t"; break;
      case '/':  output += "\\/"; break;
      default:
        if ((uint8_t)c <= 0x1F) {
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04x", c);
          output += buf;
        } else {
          output += c;
        }
    }
  }
  return output;
}
void handleGetRateConfig() {
  // 构造 JSON 响应字符串
  String json = "{";
  json += "\"rs485_baud\": \"" + String(RS485_BaudRate) + "\"";
  json += "}";

  // 发送响应
  server.send(200, "application/json", json);
}

void handleGetRS485Data() {
  if (RS485_Read_Data[0] == '\0') {
    // If empty, don't perform any operation and exit
    server.send(200, "application/json", "[]");  // Respond with an empty JSON array
    return;
  }
  // String safeString = String(RS485_Read_Data);
  String safeString = escapeJson(RS485_Read_Data);
  memset(RS485_Read_Data,0, RS485_Received_Len+1);
  RS485_Received_Len = 0;
  RS485_Read_Data[0] = '\0'; // This sets the first character to null, effectively clearing the array
  // safeString.replace("\\", "\\\\");
  // safeString.replace("\"", "\\\"");
  // safeString.replace("\n", "\\n");
  // safeString.replace("\r", "\\r");
  // safeString.replace("\t", "\\t");
  
  String json = "[\"" + safeString + "\"]";
  server.send(200, "application/json", json);
}

static uint8_t parseByteArg(const char *name)
{
  int value = server.hasArg(name) ? server.arg(name).toInt() : 0;
  if (value < 0) {
    value = 0;
  } else if (value > 255) {
    value = 255;
  }
  return (uint8_t)value;
}

void handleLightSetConfig() {
  uint8_t gpio = LIGHT_DEFAULT_GPIO;
  uint16_t count = LIGHT_DEFAULT_COUNT;
  if (server.hasArg("gpio")) {
    gpio = (uint8_t)server.arg("gpio").toInt();
  }
  if (server.hasArg("count")) {
    count = (uint16_t)server.arg("count").toInt();
  }

  Light_SetConfig(gpio, count);
  server.send(200, "application/json", Light_GetStatusJson());
}

void handleLightSetColor() {
  uint8_t r = parseByteArg("r");
  uint8_t g = parseByteArg("g");
  uint8_t b = parseByteArg("b");

  Light_SetColor(r, g, b);
  server.send(200, "application/json", Light_GetStatusJson());
}

void handleLightOff() {
  Light_Off();
  server.send(200, "application/json", Light_GetStatusJson());
}

void handleLightStatus() {
  server.send(200, "application/json", Light_GetStatusJson());
}

void handleAudioPlayMusic() {
  bool ok = Audio_ToggleMusic();
  String json = Audio_GetStatusJson();
  if (!ok && Audio_IsBusy()) {
    json.replace("\"status\":\"", "\"status\":\"audio busy - ");
  }
  server.send(200, "application/json", json);
}

void handleAudioRecord5s() {
  bool ok = Audio_Record5sAndPlay();
  String json = Audio_GetStatusJson();
  if (!ok && Audio_IsBusy()) {
    json.replace("\"status\":\"", "\"status\":\"audio busy - ");
  }
  server.send(200, "application/json", json);
}

void handleAudioStatus() {
  server.send(200, "application/json", Audio_GetStatusJson());
}

void handleRS485SetBaudRate() {
  char Text[1000];
  if (server.hasArg("data")) {
    String newData = server.arg("data");
    newData.toCharArray(Text, sizeof(Text));
  }
  server.send(200, "text/plain", "OK");

  printf("Text=%s.\r\n",Text);
  
  bool ret = ParseRS485BaudRateConfig(Text,&RS485_BaudRate);
  if(ret){
    RS485_UpdateBaudRate(RS485_BaudRate);
  }
  server.send(200, "text/plain", "OK");
}
void handleRS485SetConfig() {
  char Text[1000];
  if (server.hasArg("data")) {
    String newData = server.arg("data");
    newData.toCharArray(Text, sizeof(Text));
  }
  server.send(200, "text/plain", "OK");

  printf("Text=%s.\r\n",Text);
  
  ParseRS485Config(Text,&RS485_Read_Data_Type);
  server.send(200, "text/plain", "OK");
}
void handleRS485Send() {
  char Text[1000];
  if (server.hasArg("data")) {
    String newData = server.arg("data");
    newData.toCharArray(Text, sizeof(Text));
  }
  server.send(200, "text/plain", "OK");

  printf("Text=%s.\r\n",Text);
  RS485_Receive RS485Data;
  ParseRS485Data(Text, &RS485Data);
  SetData(RS485Data.Read_Data, RS485Data.DataLength);
  
  server.send(200, "text/plain", "OK");
}
void WIFI_Init()
{
  WiFi.mode(WIFI_AP);                             
  // WiFi.setSleep(true);    
  while(!WiFi.softAP(ssid, password)) {
    printf("Soft AP creation failed.\r\n");
    printf("Try setting up the WIFI again.\r\n");
  } 
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); // Set the IP address and gateway of the AP
  delay(100);  
  
  IPAddress myIP = WiFi.softAPIP();
  uint32_t ipAddress = WiFi.softAPIP();
  printf("AP IP address: ");
  sprintf(ipStr, "%d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
  printf("%s\r\n", ipStr);
      
  server.on("/", handleRoot);            // Relay Control page
  server.on("/getRateConfig"     , handleGetRateConfig);
  server.on("/RS485SetBaudRate" , handleRS485SetBaudRate);
  server.on("/RS485SetConfig"   , handleRS485SetConfig);
  server.on("/RS485Send"        , handleRS485Send);
  server.on("/getRS485Data"     , handleGetRS485Data);
  server.on("/LightSetConfig"   , handleLightSetConfig);
  server.on("/LightSetColor"    , handleLightSetColor);
  server.on("/LightOff"         , handleLightOff);
  server.on("/LightStatus"      , handleLightStatus);
  server.on("/AudioPlayMusic"   , handleAudioPlayMusic);
  server.on("/AudioRecord5s"    , handleAudioRecord5s);
  server.on("/AudioStatus"      , handleAudioStatus);
  
  server.begin(); 
  printf("Web server started\r\n");  
  xTaskCreatePinnedToCore(
    WebTask,    
    "WebTask",   
    4096,                
    NULL,                 
    4,                   
    NULL,                 
    0                   
  );
}


void WebTask(void *parameter) {
  while(1){
    server.handleClient(); // Processing requests from clients
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  vTaskDelete(NULL);
}

int hexCharToByte(char high, char low) {
    int highValue = (high >= '0' && high <= '9') ? (high - '0') : (high >= 'A' && high <= 'F') ? (high - 'A' + 10) : (high - 'a' + 10);
    int lowValue = (low >= '0' && low <= '9') ? (low - '0') : (low >= 'A' && low <= 'F') ? (low - 'A' + 10) : (low - 'a' + 10);
    return (highValue << 4) | lowValue;  // 将高4位和低4位合并为一个字节
}
// String decoding
bool ParseRS485BaudRateConfig(const char* Text,  unsigned long * RS485_BaudRate) {    
  int ret;
  // Parse Serial Data Type: char / hex
  ret = sscanf(strstr(Text, "RS485 BaudRate: "), "RS485 BaudRate: %lu", RS485_BaudRate);
  if (ret != 1) {
    printf("Error parsing RS485 Read Type\n");
    return false;
  }
  return true;
}
// String decoding
bool ParseRS485Config(const char* Text,uint8_t* RS485_Read_Data_Type) {    
  int ret;
  // Parse Serial Data Type: char / hex
  ret = sscanf(strstr(Text, "Data Type: "), "Data Type: %hhu", RS485_Read_Data_Type);
  if (ret != 1) {
    printf("Error parsing RS485 Read Type\n");
    return false;
  }
  return true;
}
// String decoding
bool ParseRS485Data(const char* Text, RS485_Receive* RS485Data) {    
  int ret;
  // Parse Serial Data Type: char / hex
  ret = sscanf(strstr(Text, "Data Type: "), "Data Type: %hhu", &RS485Data->DataType);
  if (ret != 1) {
    printf("Error parsing Serial Type\n");
    return false;
  }
  
  // Parse Serial Data Length: 
  const char* start = strstr(Text, "RS485 Data: ");
  if (!start) {
    printf("RS485 Data not found\n");
    return false;
  }
  start += strlen("RS485 Data: ");
  const char* end = strstr(start, "  Web End");
  if (!end) {
    printf("Data Type not found\n");
    return false;
  }
  RS485Data->DataLength = end - start;

  if(RS485Data->DataType){
    size_t rawLen = end - start;
    char* cleanHex = (char*)malloc(rawLen + 1); 
    strncpy(cleanHex, start, rawLen);
    cleanHex[rawLen] = '\0';

    char* src = cleanHex;
    char* dst = cleanHex;
    while (*src) {
      if (*src != ' ') {
          *dst++ = *src;
      }
      src++;
    }
    *dst = '\0'; 
    size_t hexLen = strlen(cleanHex);
    bool missing = 0;
    if (hexLen % 2) {
      hexLen += 1;
      missing = 1;
    }
    RS485Data->DataLength = hexLen / 2;
    RS485Data->Read_Data = (uint8_t*)malloc(RS485Data->DataLength);
    if (missing) {
      for (size_t i = 0; i < RS485Data->DataLength - 1; i++) {
        RS485Data->Read_Data[i] = hexCharToByte(cleanHex[2 * i], cleanHex[2 * i + 1]);
      }
      RS485Data->Read_Data[RS485Data->DataLength - 1] = hexCharToByte(cleanHex[2 * (RS485Data->DataLength - 1)], '0');
    } 
    else {
      for (size_t i = 0; i < RS485Data->DataLength; i++) {
        RS485Data->Read_Data[i] = hexCharToByte(cleanHex[2 * i], cleanHex[2 * i + 1]);
      }
    }
    free(cleanHex);  
  }
  else{
    // Parse actual Serial Data (e.g., char or hex values)
    RS485Data->Read_Data = (uint8_t *)malloc(RS485Data->DataLength);
    memcpy(RS485Data->Read_Data, start, RS485Data->DataLength);
  }
  return true;
}
