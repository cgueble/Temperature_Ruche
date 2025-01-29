

/**
 * Mesures de températures avec max 20 capteurs DS18B20 sur un même bus 1-Wire.
 * C GUEBLE Jan 2025
 * V1.1
 */
 
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SD.h>

/* Broche du bus 1-Wire */
 
#define ONE_WIRE_BUS 14  // Broche numérique 14, à modifier si nécessaire

const int chipSelect = SDCARD_SS_PIN; 
const boolean SERIAL_PORT_LOG_ENABLE = false; //true pour avoir la console active et false pour la desactiver ; il faut la desactiver pour l'application car meme pour que "verrou"
String logFileName = "temp.txt";
String sensorListFile = "List.txt";
bool isSensorListEmpty = "true";
const long maxLinePerFile = 100000 ;//nombre de ligne maximum par fichier 100000 (ca fait 1 ans a raison de D'une mesure par  5 minutes)
long countLinePerFile = 0;
/* Création de l'objet OneWire et sensors */
 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
#define MaxNumberOfDevices 20
byte totalDevices = 0;
bool SDCard = false;
/* Adresses des capteurs de température */
byte allAddress [MaxNumberOfDevices][8];//Permet d'explorer tous les sensors, cette liste peut changer l'ordre, elle ne sert qu'a explorer
byte allDeviceAddress [MaxNumberOfDevices][8];//tableau des adresse des sensor qui sera lu de la carte SD, c'est ce tableau qui fera foi
float SensorTemp [MaxNumberOfDevices];//tableau des mesures courantes

/* résolution definition */

int resolution = 9;  // nombre de bits
/*
9 bits   0.5 °C     93.75 ms
10 bits  0.25 °C    187.5 ms
11 bits  0.125 °C   375 ms
12 bits  0.0625 °C  750 ms */
 
 
/* Pour faire des mesures à intervalle régulier */
unsigned long tempsLed = 0; 
unsigned long tempsPrecedent = 0;
const float intervalle = 5 * 60000;   // durée entre deux mesures en ms (à modifier si nécessaire)
 

void setup() {
  countLinePerFile = maxLinePerFile;
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);  // choix de la vitesse
  delay(5000);
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("*******************************************************************");  
  Serial.println("C. GUEBLE pour T. GAIDOT - Jan 2025");
  Serial.println("pour temp Sensor DS18B20");
  Serial.println("DS18B20 => Vcc 3v");
  Serial.println("DS18B20 => GND");
  Serial.println("DS18B20 => Data Pin 14 with 4.7k pull ext to be added to Vcc");
  Serial.println("max 20 temp Sensor in parallele");
  Serial.println("*******************************************************************");
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("Start of setup()  ");
  initSDCard();
  readFileSensor(sensorListFile);//charge les valeurs des adresses de la SD card dans le tableau allSensorList
  initSensors();//make the list of sensor connected
  allSensorListUpdate();//update allSensorList avec les adresses reeles découvertes en gardant aux même places les sensor reel trouvé
  writeSensorListToSD();
  getAllTemperature();
  delay(1000);
  printAllTemperatures();//print in the console the temperature of all sensors present on the On Wire
  tempsPrecedent = millis();
  Serial.println("");
  Serial.print("Interval de mesure (minutes)= ");  
  Serial.println(intervalle/60000);
  Serial.println("");
  sensors.requestTemperatures(); //lance la lecture des sensors
  getAllTemperature();
  if (SDCard){
    writeTemperatureInSDCard();//write in the SDcard ALL sensors temperature in a file called "temp"
  } 
}
 
void loop(){
  unsigned long tempsCourant = millis();  // cette variable contient le nombre de millisecondes depuis que le programme courant a démarré. 
  if (tempsCourant < tempsPrecedent){//pour eviter les problèmes de fin de compteur de millis()
    tempsPrecedent = 0;// on est dans le cas ou millis est repassé par zero => on remet tempsPrecedent à 0
  }
  if ((tempsCourant - tempsLed) >= ((intervalle -(tempsCourant - tempsPrecedent))/100)){
    //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    tempsLed=millis();
  }
  if (tempsCourant - tempsPrecedent >= intervalle) {
    
    digitalWrite(LED_BUILTIN, HIGH);tempsPrecedent = tempsCourant;
    sensors.requestTemperatures(); //lance la lecture des sensors
    getAllTemperature();
    if (SDCard){
      writeTemperatureInSDCard();//write in the SDcard ALL sensors temperature in a file called "temp"
      countLinePerFile --;
    }             
  }
  if(countLinePerFile<=0){
    File dataFile = SD.open(logFileName, FILE_WRITE);
    if (dataFile) {
    //String dataString = "";
    //dataString += String(SensorTemp[i]);
    //dataString += ",";
    dataFile.println("");
    dataFile.println("Nombre maximum de ligne atteint => creation d'un nouveau fichier");
    dataFile.close();
    Serial.print("Nombre maximum de ligne atteint => creation d'un nouveau fichier");
    setup();
    }
  }
}

void initSensors(){
    Serial.println("Start of initSensors()");

  //Init des sensors
  //Initialize the Temperature measurement library
  oneWire.reset();
  sensors.begin();
  totalDevices = discoverOneWireDevices();         // get addresses of our one wire devices into allAddress array 
  Serial.print("totalDevices= ");
  Serial.println(totalDevices);

  
  for (byte i=0; i < MaxNumberOfDevices; i++){
    if(i < totalDevices) {
      if (SERIAL_PORT_LOG_ENABLE) {
        Serial.print("sensors.setResolution(");
        Serial.print(i);  
        Serial.print(",");
        Serial.print(resolution);
        Serial.println(")");
      }
      sensors.setResolution(allAddress[i], resolution);      // and set the ADC conversion resolution of each.
    }
  }
  Serial.println("Sensors initialized");  
}

byte discoverOneWireDevices() {
  byte j=0;                                        // search for one wire devices and
                                                   // copy to device address arrays.
  while ((j < MaxNumberOfDevices) && (oneWire.search(allAddress[j]))) {        
    j++;
  }
  if (SERIAL_PORT_LOG_ENABLE) {
    Serial.println("List of Device discovered - random order");
    for (byte i=0; i < j; i++) {
      
      Serial.print("Device ");
      Serial.print(i);  
      Serial.print(": ");                          
      printAddress(allAddress[i]);                  // print address from each device address arry.
      Serial.print("\r\n");
    }
    Serial.print("\r\n");
  }
  return j                      ;                 // return total number of devices found.
}//END discoverOneWireDevices()

void printAddress(DeviceAddress addr) {
  byte i;
  for( i=0; i < 8; i++) {                         // prefix the printout with 0x
      Serial.print("0x");
      if (addr[i] < 16) {
        Serial.print('0');                        // add a leading '0' if required.
      }
      Serial.print(addr[i], HEX);                 // print the actual value in HEX
      if (i < 7) {
        Serial.print(", ");
      }
    }
  //Serial.print("\r\n");
}



void getAllTemperature(){
  for (int i=0 ; i < MaxNumberOfDevices ; i++){
    float tempC = sensors.getTempC(allDeviceAddress[i]);           // read the device at addr.
    SensorTemp[i] = tempC;
  }
}//END getAllTemperature

void printAllTemperatures(){
  String dataString ="";  
  for (int i=0 ; i < totalDevices ; i++){
    printTemperature(i);
  }
}//END printAllTemperatures

void printTemperature(int i) {
  float tempC = 0;           // read the device at addr.
  tempC = SensorTemp[i];
  if (tempC == -127.00) {
    Serial.println("Error getting temperature");
  } else {
      Serial.print("SensorTemp n°");
      Serial.print(i);
      Serial.print(" :");
      printAddress(allDeviceAddress[i]);                  // print address from each device address arry.
      Serial.print(" = ");
      Serial.print(tempC);                          // and print its value.
      Serial.println("°C ");
  }
}//END printTemperature

float readTemperature(DeviceAddress addr) {
  float tempC = sensors.getTempC(addr);           // read the device at addr.
  if (tempC == -127.00) {
    Serial.print("Error getting temperature of device n°");
    printAddress(addr);
    Serial.print("\r\n");
  } 
  return tempC;
}//END readTemperature

void initSDCard(){
  Serial.println("Initializing SD card...");
  // see if the card is present and can be initialized:
  SDCard = SD.begin(chipSelect);
  if (!SDCard) {
    Serial.println("Card failed, or not present");
    // don't do anything more:createFile
  }else
  {
    Serial.println("card initialisation start.");
    logFileName = createFile(logFileName);//nom du fichier qui se verra rajouté un chiffre si il existe déjà
    Serial.print("New file name :    ");
    Serial.println(logFileName);
    String dataString = "";
    File logFile = SD.open(logFileName, FILE_WRITE);
    if (logFile) {
      for (int i=0 ; i < MaxNumberOfDevices ;i++){
        dataString += "temp " ;
        dataString += String(i);
        dataString += ",";
      }
      logFile.println(dataString);
      //Serial.println(dataString);
      logFile.close();
    }
    
    createFileUnique(sensorListFile);
    
  }
}//END initSDCard()  

void writeTemperatureInSDCard(){
  if (SERIAL_PORT_LOG_ENABLE) {
    Serial.println("writeTemperatureInSDCard()");
  }  
 
  //CODE TO BE ADDED TO WRITE TEMPERATURE FILE
  File dataFile = SD.open(logFileName, FILE_WRITE);
  if (dataFile) {
    String dataString = "";
    for (int i=0 ; i < MaxNumberOfDevices ;i++){
      dataString += String(SensorTemp[i]);
      dataString += ",";
    }
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.print("error opening ");
    Serial.print(logFileName);
    Serial.println("");
  }
}

String createFile(String myFileName){
  int i = 1;
  int length = 0;
  int cursor = 0;
  String extention = "";
  String name = "";
  length = myFileName.length();
   if (SERIAL_PORT_LOG_ENABLE) { 
  Serial.print("Filelength: ");
  Serial.println(length);
   }
  cursor = length;
  while(!(myFileName.substring(cursor-1, cursor) == ".")){
    if (SERIAL_PORT_LOG_ENABLE) {  
      Serial.print("cursor: ");
      Serial.print(cursor);
      Serial.print(" ");     
      Serial.println(myFileName.substring(cursor-1, cursor));
    }
    cursor = cursor - 1;
  }
  extention = myFileName.substring(cursor, length);
  name = myFileName.substring(0, cursor-1);
  if (SERIAL_PORT_LOG_ENABLE) {
  Serial.print("extention:");
  Serial.println(extention);
  Serial.print("name:");
  Serial.println(name);
  }
  myFileName = name + "." + extention;
  while(SD.exists(myFileName)){
    myFileName = name + i + "." +extention;
  i++;
  }
  if (SERIAL_PORT_LOG_ENABLE) {
  Serial.print("New File Name: ");
  Serial.println(myFileName);
  }
  File myFile = SD.open(myFileName, FILE_WRITE);
  myFile.close();
  if (SERIAL_PORT_LOG_ENABLE) {
  Serial.println("");
  Serial.print("File: ");
  Serial.print(myFileName);
  Serial.println(" created");
  }
  return myFileName;
}//END createFile

void createFileUnique(String myFileName){
  unsigned long size = 25;
  if(!SD.exists(myFileName)){
    File myFile = SD.open(myFileName, FILE_WRITE);
    size = myFile.size();
    myFile.close();
    if (SERIAL_PORT_LOG_ENABLE) {
    Serial.println("");
    Serial.print("File: ");
    Serial.print(myFileName);
    Serial.println(" created");
    }
  }else{
    if (SERIAL_PORT_LOG_ENABLE) {
    Serial.println("");
    Serial.print("File: ");
    Serial.print(myFileName);
    Serial.println(" already existing");
    }
    File myFile = SD.open(myFileName, FILE_WRITE);
    size = myFile.size();
    myFile.close();
  }
    if (SERIAL_PORT_LOG_ENABLE) {
    Serial.print("size of File=");
    Serial.println(size);
    }
  if(size==0){//test si fichier est vide
    isSensorListEmpty = true;
    if (SERIAL_PORT_LOG_ENABLE) {
    Serial.print(myFileName);
    Serial.println(" is empty");
    }
  }else{
    if (SERIAL_PORT_LOG_ENABLE) {
    Serial.print(myFileName);
    Serial.println(" is not empty");
    }
    isSensorListEmpty = false;
  }
}

void readFile(String myFile){
  File dataFile = SD.open(myFile,FILE_READ);

  // if the file is available, read it:
  if (dataFile) {
    Serial.print("Start of reading SD card file: ");
    Serial.print(myFile);
    Serial.println("");
    while (dataFile.available()) {
      Serial.write(dataFile.read());
    }
    dataFile.close();
    if (SERIAL_PORT_LOG_ENABLE) {
    Serial.print("END of reading SD card file:");
    Serial.print(myFile);
    Serial.println("");
    }
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.print("error opening:");
    Serial.println(myFile);
  }
}//END readFile

void readFileSensor(String myFile){
  int readInt = 0;
  byte readByte = 0;
  char readChar;
  Serial.println("Start of readFileSensor");
  initTabSensors();
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(myFile,FILE_READ);

  // if the file is available, write to it:
  if (dataFile) {
    if (SERIAL_PORT_LOG_ENABLE) {
    Serial.print("Start of reading SD card file: ");
    Serial.print(myFile);
    Serial.println("");
    }
    dataFile.seek(0);//curseur au début du fichier
    while (dataFile.available()) {
      Serial.write(dataFile.read());
    }
    dataFile.seek(0);//curseur au début du fichier
    int i=0;
    while (dataFile.available()) {
      readChar = dataFile.read();
      if (SERIAL_PORT_LOG_ENABLE) {
      Serial.print("readChar=");
      Serial.println(readChar);    
      }
      if('#' == readChar){
        if (SERIAL_PORT_LOG_ENABLE) {
          Serial.println("# détécté");
        }
        for(int j=0 ; j<8 ; j++){
          readChar = dataFile.read();
          if (SERIAL_PORT_LOG_ENABLE) {
            Serial.print("readChar=");
            Serial.print(readChar);
          }
          allDeviceAddress[i][j]=nibble(readChar); //CGU le problème est là
          allDeviceAddress[i][j] = allDeviceAddress[i][j] << 4 ;
          readChar = dataFile.read();
          if (SERIAL_PORT_LOG_ENABLE) {
            Serial.print("  readChar=");
            Serial.print(readChar);
          }
          allDeviceAddress[i][j] = allDeviceAddress[i][j] + nibble(readChar);
          if (SERIAL_PORT_LOG_ENABLE) {
          Serial.print("  allDeviceAddress[i][j]=");
          Serial.println(allDeviceAddress[i][j]);
          }   
          readInt = dataFile.peek();
          if(44 == readInt){
            dataFile.read();//avance le curseur pour passer la virgule 
          }
          
        }
        i++;   
      }
    }
    dataFile.close();
    if (SERIAL_PORT_LOG_ENABLE) {
      Serial.print("END of reading SD card file:");
      Serial.print(myFile);
      Serial.println("");
    }
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.print("error opening:");
    Serial.println(myFile);
  }
}//END readFileSensor

byte nibble(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return 0;  // Not a valid hexadecimal character
}

void writeSensorListToSD(){
 if (SERIAL_PORT_LOG_ENABLE) {
    Serial.println("Start of writeSensorListToSD()");
  }  
 
  //CODE TO BE ADDED TO WRITE TEMPERATURE FILE
  File dataFile = SD.open(sensorListFile, FILE_WRITE);
  if (dataFile) {
    String dataString = "";
    for (int i=0 ; i < MaxNumberOfDevices ;i++){
      dataString += "#";
      for (int j = 0 ; j < 8 ;j++ ){
        if(allDeviceAddress[i][j] <= 16){
          dataString +="0";
        }
        dataString += String(allDeviceAddress[i][j], HEX);
        if(j<7){
          dataString += ",";
        }
      }
      //dataString += ";\n\r";
      dataString += ";\r";
    }

    dataFile.println(dataString);
    if (SERIAL_PORT_LOG_ENABLE) {
      Serial.println(dataString);
    }
    dataFile.close();
    // print to the serial port too:

  }
  // if the file isn't open, pop up an error:
  else {
    Serial.print("error opening ");
    Serial.print(sensorListFile);
    Serial.println("");
  }


}//END writeSensorListToSD

void initTabSensors(){
  if (SERIAL_PORT_LOG_ENABLE) {
  Serial.println("Mise à 0xFF de troutes les addresse de allDeviceAddress[i][j]");
  }
  for(int i=0 ; i< MaxNumberOfDevices ; i++){
    for(int j=0 ; j<8 ; j++){
      allDeviceAddress[i][j]=0xFF;
    }
  }
}//END initTabSensors()

void printTabSensors(){
  Serial.println("Status of table allDeviceAddress[i][j]");
  String dataString = "";
  for(int i=0 ; i< MaxNumberOfDevices ; i++){
    dataString += i;
    dataString += "#";
    for(int j=0 ; j<8 ; j++){
      if(allDeviceAddress[i][j] <= 16){
          dataString +="0";
      }
      dataString += String(allDeviceAddress[i][j], HEX);
      if(j<7){
        dataString += ",";
      }
      
    }
    dataString += ";";
    Serial.println(dataString);
    dataString = "";
  }
}//END printTabSensors

void allSensorListUpdate(){
  bool validSensors[MaxNumberOfDevices];
  bool newDevice[MaxNumberOfDevices];
  int countGoodAddress = 0;
  int adressAt00Count = 0;
  for(int l=0 ;l<MaxNumberOfDevices ; l++){
    validSensors[l]=false;
    newDevice[l]=true;    
  }
  SD.remove(sensorListFile);//effece le fichier de sensors
  createFileUnique(sensorListFile);
  for(int i=0 ; i<MaxNumberOfDevices ; i++){
    if(!validSensors[i]){
      for(int k=0 ; k<MaxNumberOfDevices ; k++){
        if(newDevice[k] && !validSensors[i]){
          countGoodAddress = 0;
          adressAt00Count = 0;
          for(int j =0 ; j<8 ; j++){
            if(allDeviceAddress[i][j]==allAddress[k][j]){
              countGoodAddress ++;
            }  
            if(allAddress[k][j]==0x00){
            adressAt00Count ++;
            }  
          }
          if (SERIAL_PORT_LOG_ENABLE) {
            Serial.print("allDeviceAddress i=");
            for(int l=0 ;l<8 ; l++){
              Serial.print(allDeviceAddress[i][l],HEX);
            }
            Serial.println("");
            Serial.print("allAddress       k=");
            for(int l=0 ;l<8 ; l++){
              Serial.print(allAddress[k][l],HEX);
            }
            Serial.print("    ");

            Serial.print("allDeviceAddress i=");
            Serial.print(i);
            Serial.print(" vs allAddress k=");
            Serial.print(k);
            Serial.print("  countGoodAddress=");
            Serial.print(countGoodAddress);
            Serial.print("  adressAt00Count=");
            Serial.print(adressAt00Count);
          }
          if(countGoodAddress==8){
            validSensors[i] = true;
            newDevice[k] = false;
          }else{
            validSensors[i] = false;
          }
          if(adressAt00Count==8){
            newDevice[k] = false;
          }else{
           
          }
          if (SERIAL_PORT_LOG_ENABLE) {
            Serial.print("  validSensors[");
            Serial.print(i);
            Serial.print("]=");
            Serial.print(validSensors[i]);
            Serial.print("  newDevice[");
            Serial.print(k);
            Serial.print("]=");
            Serial.println(newDevice[k]);
          }
        }
      }
    } 
  }
  if (SERIAL_PORT_LOG_ENABLE) {
    for(int l=0 ;l<MaxNumberOfDevices ; l++){
      Serial.print("validSensors[i] ");
      Serial.print(validSensors[l]);
      Serial.print("    newDevice[k] ");
      Serial.println(newDevice[l]);    
    }
  }
  for(int i=0 ; i<MaxNumberOfDevices ; i++){
    for(int k=0 ; k<MaxNumberOfDevices ; k++){
      if(!validSensors[i] & newDevice[k]){
        for(int j =0 ; j<8 ; j++){
          allDeviceAddress[i][j] = allAddress[k][j];
        }
        newDevice[k]=false;
      }
    }
  }
}//END allSensorListUpdate

void goToSleep(){
    if (SERIAL_PORT_LOG_ENABLE) {
          Serial.println("");
          Serial.println("Time to go to sleep");
          }
    //deepSleep(5000);//endless sleep until    trouver la commande sleep pour MKRZERO
}//END goTOsleep