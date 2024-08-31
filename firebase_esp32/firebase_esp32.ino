
# include <WiFi.h>   // To connect esp32 to internet
# include <Firebase_ESP_Client.h>   // Interface ESP32 with firebase
# include "addons/TokenHelper.h"  // for token geenrations
# include "addons/RTDBHelper.h"  // realtime databese loading helper

# define WIFI_SSID "Your WiFi SSID"  // my mobile hotspot
# define WIFI_PASSWORD "Your WiFi Password" 

# define API_KEY "Your Firebase database API"  
# define DATABASE_URL "Your Firebase URL"

#define RXD2 16   // Receiving Pin of UART
#define TXD2 17   // Transmitte Pin of UART


// Firebase objects
/////////////////////////////////////////////////////////
// firebase objects to handle data paths
FirebaseData fbdo, fbdo_edit_mode , fbdo_h_set , fbdo_li_set , fbdo_sm_set , fbdo_t_set;
/////////////////////////////////////////////////////////
FirebaseAuth auth;  // authenticatio purpose
FirebaseConfig config; // configuration purposes
/////////////////////////////////////////////////////////
unsigned long sendDataPreMillis = 0;
bool signupOK = false ; 
/////////////////////////////////////////////////////////


// Variables for UART Communication

// Golbal Vaariables
char c;
String dataIn;

int8_t indexOfA , indexOfB , indexOfC , indexOfD , indexOfE ;

String data1 , data2 , data3 , data4 , data5 ;



// variables for send data to firebase///////////////////
/////////////////////////////////////////////////////////

// AIR quality  levels in ppm
String coNow  ; // CO gas 
String lpNow  ; // LP level

// HUMIDITY
String hNow  ;
int hSet = 0 ;

// Light Intensity
String liNow ;
int liSet = 10 ;

// Soil Moisture
String smNow  ;
int smSet = 12 ; 

// Temperature
String tNow  ;
int tSet = 14 ;

// edit mode 
bool editMode = false;
// variables for recive data from firebase


void setup(){

  // connecting wifi.......
  Serial.begin(115200);

   //Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2); with ATMEGA32A
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Serial Txd is on pin: "+String(TX));
  Serial.println("Serial Rxd is on pin: "+String(RX));

  /// WIFI connecting
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  Serial.println("Connecting to WiFi Network");
  while (WiFi.status() != WL_CONNECTED){
    for (int i=0;i<2; i=i+1){
      Serial.print("----Trying To Connect WiFi Network----");
      delay(500);
    }
    Serial.println();
  }

  Serial.println();
  Serial.println("Succesfully Connected to WiFi Network");
  Serial.print("IP Address : ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY ;
  config.database_url = DATABASE_URL ;

  if (Firebase.signUp(&config,&auth,"","")){
    Serial.println("Sign Up OK");
    signupOK = true ;

  }
  else{
    Serial.printf("%s\n",config.signer.signupError.message.c_str());

  }

  config.token_status_callback = tokenStatusCallback ;
  Firebase.begin(&config ,&auth);
  Firebase.reconnectWiFi(true);

  ////////////////////STREAM//////////////////////////////////
  // begining of the stream function

  // edit mode
  if(! Firebase.RTDB.beginStream(&fbdo_edit_mode, "/PROJECT_1/EDIT_MODE")){
    Serial.printf("Stream ---EDIT MODE--- begin Error, %s\n\n",fbdo_edit_mode.errorReason().c_str());
  }

  // humidity
  if(! Firebase.RTDB.beginStream(&fbdo_h_set, "/PROJECT_1/SENSOR_VALUES/HUMIDITY/H_SET")){
    Serial.printf("Stream ---HUMIDITY--- begin Error, %s\n\n",fbdo_h_set.errorReason().c_str());
  }

  // light intensity
  if(! Firebase.RTDB.beginStream(&fbdo_li_set, "/PROJECT_1/SENSOR_VALUES/LIGHT_INTENSITY/LI_SET")){
    Serial.printf("Stream ---LIGHT INTENSITY--- begin Error, %s\n\n",fbdo_li_set.errorReason().c_str());
  }  

  // soil moisture
  if(! Firebase.RTDB.beginStream(&fbdo_sm_set, "/PROJECT_1/SENSOR_VALUES/SOIL_MOISTURE/SM_SET")){
    Serial.printf("Stream ---SOIL MOISTURE--- begin Error, %s\n\n",fbdo_sm_set.errorReason().c_str());
  }

  // temperature
  if(! Firebase.RTDB.beginStream(&fbdo_t_set, "/PROJECT_1/SENSOR_VALUES/TEMPERATURE/T_SET")){
    Serial.printf("Stream ---TEMPERATURE--- begin Error, %s\n\n",fbdo_t_set.errorReason().c_str());
  }

}

void loop(){

    /////////////////////////////////Methods for set/initialize the parameter values//////////////////////////////////////////
  while (Serial2.available()) {
    c = Serial2.read();
    if (c == '\n'){
      break; 
    }else {
      dataIn += c ;
    }
    //Serial.print(char(Serial2.read()));
  }

  if ( c =='\n'){
    Parse_the_Data();

     //show all data in serial monitor
    Serial.println("Data1 = " + tNow);
    Serial.println("Data2 = " + hNow);
    Serial.println("Data3 = " + smNow);
    Serial.println("Data4 = " + liNow);
    Serial.println("Data5 = " + coNow);
    Serial.println("===================================================");


    c = 0; //reset the variable
    dataIn = "";
  }

  // the data willl be sent to the FireBase each and every five seconds, or when the restaring of this loop. 
  if (Firebase.ready() && signupOK && (millis() - sendDataPreMillis > 2000 || sendDataPreMillis == 0)){
    sendDataPreMillis = millis();

   ///////////////////////////////////////EXAMPLE////////////////////////////////////////////////////////////////////////////
    // -------STORE sensor data to a RTDB 
    //ldrData = analogRead(LDR_PIN);
    //voltage = (float)analogReadMilliVolts(LDR_PIN)/1000;


   /////////////////////////////////////Upload PARAMETER values to the FireBase/////////////////////////////////////////////////////////////////////


    // AIR QUALITY -- CO Level //

    if (Firebase.RTDB.setString(&fbdo,"/PROJECT_1/SENSOR_VALUES/AIR_QUALITY/CO_GAS/CO_NOW",coNow)){
      //Serial.println();
      Serial.print("System CO Gas Level : ");
      Serial.print(coNow);
      Serial.print("  (" + fbdo.dataType()+ ")");
      Serial.println(" - - - Succesfully Saved to: " + fbdo.dataPath());
      
    }
    else{
      Serial.println("FAILED to Update the System CO gas Level - ERROR: "+fbdo.errorReason());
    }


    // AIR QUALITY -- LP gas Level  //
/*
    if (Firebase.RTDB.setString(&fbdo,"/PROJECT_1/SENSOR_VALUES/AIR_QUALITY/LP_GAS/LP_NOW",lpNow)){
      //Serial.println();
      Serial.print("System LP Gas Level : ");
      Serial.print(lpNow);
      Serial.print("  (" + fbdo.dataType()+ ")");
      Serial.println(" - - - Succesfully Saved to: " + fbdo.dataPath());
      
    }
    else{
      Serial.println("FAILED to Update the System LP Gas Level - ERROR: "+fbdo.errorReason());
    }

*/
    // HUMIDITY //

    if (Firebase.RTDB.setString(&fbdo,"/PROJECT_1/SENSOR_VALUES/HUMIDITY/H_NOW",hNow)){
      //Serial.println();
      Serial.print("System Humidity : ");
      Serial.print(hNow);
      Serial.print("  (" + fbdo.dataType()+ ")");
      Serial.println(" - - - Succesfully Saved to: " + fbdo.dataPath());
      
    }
    else{
      Serial.println("FAILED to Update the System Humidity - ERROR: "+fbdo.errorReason());
    }


    // LIGHT INTENSITY //

    if (Firebase.RTDB.setString(&fbdo,"/PROJECT_1/SENSOR_VALUES/LIGHT_INTENSITY/LI_NOW",liNow)){
      //Serial.println();
      Serial.print("System Light Intensity : ");
      Serial.print(liNow);
      Serial.print("  (" + fbdo.dataType()+ ")");
      Serial.println(" - - - Succesfully Saved to: " + fbdo.dataPath());
      
    }
    else{
      Serial.println("FAILED to Update the System Light Intenisty - ERROR: "+fbdo.errorReason());
    }



    // SOIL MOISTURE //

    if (Firebase.RTDB.setString(&fbdo,"/PROJECT_1/SENSOR_VALUES/SOIL_MOISTURE/SM_NOW",smNow)){
      //Serial.println();
      Serial.print("System Soil Moisture : ");
      Serial.print(smNow);
      Serial.print("  (" + fbdo.dataType()+ ")");
      Serial.println(" - - - Succesfully Saved to: " + fbdo.dataPath());
      
    }
    else{
      Serial.println("FAILED to Update the System Soil Moisture - ERROR: "+fbdo.errorReason());
    }



    // TEMPERATURE //

    if (Firebase.RTDB.setString(&fbdo,"/PROJECT_1/SENSOR_VALUES/TEMPERATURE/T_NOW",tNow)){
      //Serial.println();
      Serial.print("System Temperature : ");
      Serial.print(tNow);
      Serial.print("  (" + fbdo.dataType()+ ")");
      Serial.println(" - - - Succesfully Saved to: " + fbdo.dataPath());
      
    }
    else{
      Serial.println("FAILED to Update the System Temperature - ERROR: "+fbdo.errorReason());
    }


  }
  
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Read the data from a RTDB on data change
  if (Firebase.ready() && signupOK){

    // Set the  Edit mode enable or not
    if(!Firebase.RTDB.readStream(& fbdo_edit_mode)){
      Serial.printf("Steram h_set read ERROR , %s\n\n",fbdo_edit_mode.errorReason().c_str());
    }
    if (fbdo_edit_mode.streamAvailable() ) {

      if (fbdo_edit_mode.dataType()=="boolean"){
        editMode = fbdo_edit_mode.boolData();
        if (editMode == 1) {
          Serial.print("Successfully READ -- EDIT MODE: ON (True: Edit Mode ON) (");
        } else {
          Serial.print("Successfully READ -- EDIT MODE: OFF (False: Edit Mode OFF) (");
        }
        Serial.print(editMode);
        Serial.print(" ");
        Serial.print(fbdo_edit_mode.dataType());
        Serial.println(")");
        Serial.println(fbdo_edit_mode.dataPath());
        Serial.println(" ");

        //ledcWrite(PWMChannel,pwmValue);
      }
    }
    
    // humidity set value
    if(!Firebase.RTDB.readStream(& fbdo_h_set)){
      Serial.printf("Steram h_set read ERROR , %s\n\n",fbdo_h_set.errorReason().c_str());
    }
    if (fbdo_h_set.streamAvailable() ) {

      if (fbdo_h_set.dataType() == "int") {
        hSet = fbdo_h_set.intData();
        Serial.print("Succesfully READ --- Set up HUMIDITY : ");
        Serial.print(hSet);
        Serial.print(" (");
        Serial.print(fbdo_h_set.dataType());
        Serial.println(")");
        Serial.println(fbdo_h_set.dataPath());
        Serial.println();
      }

    }

    // light intensity set value
    if(!Firebase.RTDB.readStream(& fbdo_li_set)){
      Serial.printf("Steram li_set read ERROR , %s\n\n",fbdo_li_set.errorReason().c_str());
    } 
    if (fbdo_li_set.streamAvailable() ) {

      if (fbdo_li_set.dataType() == "int") {
        liSet = fbdo_li_set.intData();
        Serial.print("Successfully READ --- Set up LIGHT INTENSITY : ");
        Serial.print(liSet);
        Serial.print(" (");
        Serial.print(fbdo_li_set.dataType());
        Serial.println(")");
        Serial.println(fbdo_li_set.dataPath());
        Serial.println();
      }

    }

    // Soil Moisture set value
    if(!Firebase.RTDB.readStream(& fbdo_sm_set)){
      Serial.printf("Steram sm_set read ERROR , %s\n\n",fbdo_sm_set.errorReason().c_str());
    }
    if (fbdo_sm_set.streamAvailable() ) {

      if (fbdo_sm_set.dataType() == "int") {
        smSet = fbdo_sm_set.intData();
        Serial.print("Successfully READ --- Set up SOIL MOISTURE : ");
        Serial.print(smSet);
        Serial.print(" (");
        Serial.print(fbdo_sm_set.dataType());
        Serial.println(")");
        Serial.println(fbdo_sm_set.dataPath());
        Serial.println();
      }

    }

    // Temperature Set value
    if(!Firebase.RTDB.readStream(& fbdo_t_set)){
      Serial.printf("Steram t_set read ERROR , %s\n\n",fbdo_t_set.errorReason().c_str());
    }
    if (fbdo_t_set.streamAvailable() ) {

      if (fbdo_t_set.dataType() == "int") {
        tSet = fbdo_t_set.intData();
        Serial.print("Successfully READ --- Set up TEMPERATURE : ");
        Serial.print(tSet);
        Serial.print(" (");
        Serial.print(fbdo_t_set.dataType());
        Serial.println(")");
        Serial.println(fbdo_t_set.dataPath());
        Serial.println();
      }

    }
  }

}

////////////////////////////////////////////////////////////////////////////////////////////////
void Parse_the_Data()
{
  indexOfA = dataIn.indexOf("A");
  indexOfB = dataIn.indexOf("B");
  indexOfC = dataIn.indexOf("C");
  indexOfD = dataIn.indexOf("D");
  indexOfE = dataIn.indexOf("E");

  tNow = dataIn.substring(0,indexOfA);
  hNow = dataIn.substring(indexOfA+1,indexOfB);
  smNow = dataIn.substring(indexOfB+1,indexOfC);
  liNow = dataIn.substring(indexOfC+1,indexOfD);
  coNow = dataIn.substring(indexOfD+1,indexOfE);

}























