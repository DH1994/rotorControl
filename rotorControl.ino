#include <WiFi.h>
#include <AccelStepper.h>

#include <ElegantOTA.h>

AccelStepper stepper(AccelStepper::FULL4WIRE, 7, 9, 8, 10);

const char *ssid = "";
const char *password = "";

WebServer server(80);
NetworkServer cmdServer(8080);

unsigned long ota_progress_millis = 0;

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  cmdServer.begin();

  stepper.setMaxSpeed(150);
  stepper.setSpeed(150);
  stepper.setAcceleration(100.0);

  server.on("/", []() {
    server.send(200, "text/plain", "Hi! This is ElegantOTA Demo.");
  });

  ElegantOTA.begin(&server);  // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
}

int splitCmd(String cmd, String *args) {
  int argCt = 0;
  while (cmd.length() > 0) {
    int index = cmd.indexOf(' ');
    if (index == -1)  // No space found
    {
      args[argCt++] = cmd;
      break;
    } else {
      args[argCt++] = cmd.substring(0, index);
      cmd = cmd.substring(index + 1);
    }
  }
  return argCt;
}

void handleCmd(String cmdStr) {
  String cmd[20];
  int argCt = splitCmd(cmdStr, cmd);

  if (cmd[0] == "moveTo") {
    if (argCt > 1) {
      stepper.moveTo(cmd[1].toInt());
    }
  } else if (cmd[0] == "reset") {
    stepper.setCurrentPosition(0);
  } else if (cmd[0] == "set") {
    if (argCt > 1) {
      stepper.setCurrentPosition(cmd[1].toInt());
    }
  }
}

String handleClient() {
  String currentLine = "";
  NetworkClient client = cmdServer.accept();  // listen for incoming clients

  if (client) {                     // if you get a client,
    Serial.println("New Client.");  // print a message out the serial port

    while (client.connected()) {  // loop while the client's connected
      if (client.available()) {   // if there's bytes to read from the client,
        char c = client.read();   // read a byte, then
        if (c == '\n') {          // if the byte is a newline character
          // break out of the while loop:
          break;
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }

    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
  return currentLine;
}

void loop() {
  String currentLine = handleClient();

  if (currentLine.length()) {
    handleCmd(currentLine);
    Serial.println(currentLine);
  }

  if (stepper.distanceToGo()) {
    while (stepper.distanceToGo()) {
      stepper.enableOutputs();
      stepper.run();
    }
    stepper.disableOutputs();
  }

  server.handleClient();
  ElegantOTA.loop();
}
