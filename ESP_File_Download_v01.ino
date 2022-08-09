#include <sd_diskio.h>
#include <sd_defines.h>
#include <SD.h>

#include <dummy.h>

/*  Version 1
 *  
 *  ESP32/ESP8266 example of downloading a file from the device's SD Filing System
 *  
 This software, the ideas and concepts is Copyright (c) David Bird 2018. All rights to this software are reserved.
 
 Any redistribution or reproduction of any part or all of the contents in any form is prohibited other than the following:
 1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
 2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author David Bird as the source of the material.
 3. You may not, except with my express written permission, distribute or commercially exploit the content.
 4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.

 The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
 software use is visible to an end-user.
 
 THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT. FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY 
 OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 See more at http://www.dsbird.org.uk
 *
*/
#include <WiFi.h>              // Built-in
#include <strings_en.h>
#include <WiFiManager.h>
#include <WebServer.h>    // https://github.com/Pedroalbuquerque/ESP32WebServer descargar y ubicar en la librería
#include <ESPmDNS.h>
#include "FS.h"
#include "Arduino.h"
#include "Wav.h"
#include "I2S.h"
#include "Network.h"
#include "Sys_Variables.h"
#include "CSS.h"
#include <SD.h> 
#include <SPI.h>


#define I2S_MODE I2S_MODE_ADC_BUILT_IN


WebServer server(80);



const int headerSize = 44;
//const int waveDataSize = record_time * 88000;
const int numCommunicationData = 8000;
const int numPartWavData = numCommunicationData/4;
byte header[headerSize];
char communicationData[numCommunicationData];
char partWavData[numPartWavData];
File file;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void setup(void){
  pinMode(22,OUTPUT);
  Serial.begin(115200);
  if (!WiFi.config(local_IP, gateway, subnet, dns)) { //WiFi.config(ip, gateway, subnet, dns1, dns2);
    Serial.println("Falló la configuración de WiFi STATION "); 
  } 

WiFiManager wifiManager;  
//wifiManager.resetSettings();                     //Se utiliza la librería WiFi Manager para permitir al usuario configurar el nombre y contraseña de la red
wifiManager.autoConnect("ESP32-Ucuenca-WiFi");     //Nombre de la red del ESP32 como AP


digitalWrite(22,HIGH); // Cuando la conexión es satisfactoria el pin 22 se pone en alto.
  
  Serial.println("\nConectado a "+WiFi.SSID()+" Usando la dirección IP: "+WiFi.localIP().toString()); // Reporte de que la conexión ha sido existosa
 
  if (!MDNS.begin(servername)) {           // Para la conexión también se puede utilizar  http://ucuenca.local configurado en Network
    Serial.println(F("¡Error al configurar la respuesta MDNS!")); 
    ESP.restart(); 
  } 
  #ifdef ESP32
    Serial.println(MISO);
    pinMode(19,INPUT_PULLUP);
  #endif
  Serial.print(F("Inicializando tarjeta SD...")); 
  if (!SD.begin(SD_CS_pin)) { // Comprueba si la tarjeta está presente y puede ser inicializada 
    Serial.println(F("Tarjeta con error o no presente, no es posible el registro de datos de la tarjeta SD..."));
    SD_present = false; 
  } 
  else
  {
    Serial.println(F("Tarjeta inicializada... acceso a archivos habilitado..."));
    SD_present = true; 
  }
  //----------------------------------------------------------------------   
  ///////////////////////////// Comandos de servidor 
  server.on("/",         HomePage);
  server.on("/download", File_Download);
  server.on("/grabar",   Grabar_Audio);
  server.on("/delete",   File_Delete);
  server.on("/dir",      SD_dir);
  ///////////////////////////// 
  server.begin();
  Serial.println("Servidor HTTP iniciado");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void loop(void){

  
  server.handleClient(); // Listen for client connections
}

//Funciones del servidor web
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void HomePage(){
  SendHTML_Header();
  webpage += F("<a href='/download'><button>Descargar</button></a>");  
  webpage += F("<a href='/grabar'><button>Grabar</button></a>");
  webpage += F("<a href='/delete'><button>Borrar</button></a>");
  webpage += F("<a href='/dir'><button>Directorio</button></a>"); 
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop(); 
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Download(){ // Esto se llama dos veces, el primer paso selecciona la entrada, el segundo paso luego procesa los argumentos de la línea de comando
  if (server.args() > 0 ) { // Argumentos recibidos
    if (server.hasArg("download")) SD_file_download(server.arg(0));
  }
  else SelectInput("Archivos para descargar","Ingrese el nombre del archivo","download","download");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Grabar_Audio(){
if (server.args() > 0 ) { // Argumentos recibidos
    if (server.hasArg("grabar")) SD_file_grabar(server.arg(0));
  }
  else SelectInput1("","Ingrese el # de audio que desea grabar","grabar","grabar");
 
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_file_download(String filename){ //Función para descargar archivo
  if (SD_present) { 
    File download = SD.open("/"+filename);
    if (download) {
      server.sendHeader("Content-Type", "text/text");
      server.sendHeader("Content-Disposition", "attachment; filename="+filename);
      server.sendHeader("Connection", "close");
      server.streamFile(download, "application/octet-stream");
      download.close();
    } else ReportFileNotPresent("download"); 
  } else ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_file_grabar(String numtime){ //Función para grabar archivo
  String filename1 = "/sound";
  char charBuf[30];
  if (SD_present) {
    String num = getValue(numtime,',',0);
    String timer = getValue(numtime,',',1);
    int timerINT=timer.toInt();
    SendHTML_Header();
    filename1.concat(num + ".wav");
    filename1.toCharArray(charBuf,30);
    //const char filename[] = nombre;
    Serial.println("Grabación " + filename1);  
    SD.remove(filename1);
    file = SD.open(filename1, FILE_WRITE);
    grabacion(timerINT);
    Serial.println("La grabacion " + filename1 + " ha finalizado");
    webpage += "<h3>El archivo sound" +num+" se ha grabado correctamente</h3>";
    webpage += F("<a href='/grabar'>[Volver]</a><br><br>");
    append_page_footer(); 
    SendHTML_Content();
    SendHTML_Stop();
  } else ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void File_Delete(){
  if (server.args() > 0 ) { // Argumentos recibidos
    if (server.hasArg("delete")) SD_file_delete(server.arg(0));
  }
  else SelectInput("Eliminaci&oacute;n de archivos","Escriba el nombre del archivo que desee borrar","delete","delete");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_file_delete(String filename) { // Función para borrar archivo
  if (SD_present) { 
    SendHTML_Header();
    File dataFile = SD.open("/"+filename, FILE_READ); // 
    Serial.print("Borrando archivo: "); Serial.println(filename);
    if (dataFile)
    {
      if (SD.remove("/"+filename)) {
        Serial.println(F("File deleted successfully"));
        webpage += "<h3>El archivo '"+filename+"'ha sido borrado satisfactoriamente</h3>"; 
        webpage += F("<a href='/delete'>[Volver]</a><br><br>");
      }
      else
      { 
        webpage += F("<h3>El archivo no se ha borrado - error</h3>");
        webpage += F("<a href='delete'>[Volver]</a><br><br>");
      }
    } else ReportFileNotPresent("delete");
    append_page_footer(); 
    SendHTML_Content();
    SendHTML_Stop();
  } else ReportSDNotPresent();
} 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void SD_dir(){
  if (SD_present) { 
    File root = SD.open("/");
    if (root) {
      root.rewindDirectory();
      SendHTML_Header();
      webpage += F("<h3 class='rcorners_m'>Contenido de la tarjeta SD</h3><br>");
      webpage += F("<table align='center'>");
      webpage += F("<tr><th>Name/Type</th><th style='width:20%'>Type File/Dir</th><th>File Size</th></tr>");
      printDirectory("/",0);
      webpage += F("</table>");
      SendHTML_Content();
      root.close();
    }
    else 
    {
      SendHTML_Header();
      webpage += F("<h3>No se encontraron archivos</h3>");
    }
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();  
  } else ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void printDirectory(const char * dirname, uint8_t levels){ //Función para mostrar el directorio
  File root = SD.open(dirname);
  if(!root){
    return;
  }
  if(!root.isDirectory()){
    return;
  }
  File file = root.openNextFile();
  while(file){
    if (webpage.length() > 1000) {
      SendHTML_Content();
    }
    if(file.isDirectory()){
      Serial.println(String(file.isDirectory()?"Dir ":"File ")+String(file.name()));
      webpage += "<tr><td>"+String(file.isDirectory()?"Dir":"File")+"</td><td>"+String(file.name())+"</td><td></td></tr>";
      printDirectory(file.name(), levels-1);
    }
    else
    {
      //Serial.print(String(file.name())+"\t");
      webpage += "<tr><td>"+String(file.name())+"</td>";
      Serial.print(String(file.isDirectory()?"Dir ":"File ")+String(file.name())+"\t");
      webpage += "<td>"+String(file.isDirectory()?"Dir":"File")+"</td>";
      int bytes = file.size();
      String fsize = "";
      if (bytes < 1024)                     fsize = String(bytes)+" B";
      else if(bytes < (1024 * 1024))        fsize = String(bytes/1024.0,3)+" KB";
      else if(bytes < (1024 * 1024 * 1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
      else                                  fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
      webpage += "<td>"+fsize+"</td></tr>";
      Serial.println(String(fsize));
    }
    file = root.openNextFile();
  }
  file.close();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Header(){
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate"); 
  server.sendHeader("Pragma", "no-cache"); 
  server.sendHeader("Expires", "-1"); 
  server.setContentLength(CONTENT_LENGTH_UNKNOWN); 
  server.send(200, "text/html", ""); 
  append_page_header();
  server.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Content(){
  server.sendContent(webpage);
  webpage = "";  
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Stop(){
  server.sendContent("");
  server.client().stop(); 
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SelectInput(String heading1, String heading2, String command, String arg_calling_name){
  SendHTML_Header();
  webpage += F("<h3 class='rcorners_m'>");webpage += heading1+"</h3><br>";
  webpage += F("<h3>"); webpage += heading2 + "</h3>"; 
  webpage += F("<FORM action='/"); webpage += command + "' method='post'>"; 
  webpage += F("<input type='text' name='"); webpage += arg_calling_name; webpage += F("' value=''><br>");
  webpage += F("<type='submit' name='"); webpage += arg_calling_name; webpage += F("' value=''><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SelectInput1(String heading1,String heading2, String command, String arg_calling_name){
  SendHTML_Header();
  webpage += F("<h3 class='rcorners_m'>");webpage += heading1+"</h3><br>";
  webpage += F("<h3>"); webpage += heading2 + "</h3>"; 
  webpage += F("<FORM action='/"); webpage += command + "' method='post'>"; 
  webpage += F("<input type='text' name='"); webpage += arg_calling_name; webpage += F("' value=''><br>");
  webpage += F("<type='submit' name='"); webpage += arg_calling_name; webpage += F("' value=''><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportSDNotPresent(){
  SendHTML_Header();
  webpage += F("<h3>Tarjeta SD no insertada</h3>"); 
  webpage += F("<a href='/'>[Volver]</a><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportFileNotPresent(String target){
  SendHTML_Header();
  webpage += F("<h3>El archivo no existe</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Volver]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void grabacion(int record_time){ //Función para grabar audio
        if (!file) return;
              const int waveDataSize = record_time * 88000;
              CreateWavHeader(header, waveDataSize);
              file.write(header, headerSize);
              I2S_Init(I2S_MODE, I2S_BITS_PER_SAMPLE_32BIT);
              for (int j = 0; j < waveDataSize/numPartWavData; ++j) {
              I2S_Read(communicationData, numCommunicationData);
              for (int i = 0; i < numCommunicationData/8; ++i) {
               partWavData[2*i] = communicationData[8*i + 2];
                partWavData[2*i + 1] = communicationData[8*i + 3];
               }
              file.write((const byte*)partWavData, numPartWavData);
              }
              file.close();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
