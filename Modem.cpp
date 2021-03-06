#include "Modem.h"

  
static char strAux[50]; //!<String auxiliar para cargar el mensaje TCPSEND
static char strAux2[250]; //!<String auxiliar para cargar el mensaje TCPSEND
char* modemBufferIn;
static char ipServer[20]; 
char s[2]; //!< auxiliar .
static  struct pt pt1, pt2,pt3, pt4; //!< cada protohtread necesita uno de estos
//variables para checkResponseATcommand.
static volatile  int iATcommand=0;
//static int answer;
MODEM_STATE ESTADO_MODEM = NO_TASK_MODEM;
MODEM_STATE* OK_ERROR_MODEM;
//variables de tcp/ip
static char* ptch;
static char* pDns;
static char* ptBufferTcpOut;
//variables send sms
static char* pMensajeSms;
static char* pNumeroTel;
//variables de ftp
uint16_t puertoFtp=0;
uint16_t largoFile=0;
static char* pFile;
static char* pUser;
static char* pPwd;
static volatile unsigned long timestamp = 0;
static volatile  unsigned long timeOutModem = 0;
uint16_t volatile puerto = 0;

void resetVariables();

boolean checkResponseATcommand(char* comando ,char* respuestaEsperada){
  /*!
   * Funcion que checka la respuesta al comando enviado al modem. Esta programada para que no sea Bloqueante.
   * Para que funcione correctamente antes de llamarla hay que reiniciar los valores del contador y el buffer ya que son static.
   * Tambien tiene la posiblidad de enviar comandos( por ahora esta customizado para el comando CREG?
   */
  boolean ret = false;

// Serial.println("chekResponse"); 
 while(Serial1.available() > 0) { 
  strAux2[iATcommand] = (char)Serial1.read();    
  iATcommand++;
  if (strstr(strAux2, respuestaEsperada) != NULL){
    ret = true; 
    Serial.println("es trueee");
    Serial.println(strAux2);
    break;
    }
  }
  //Serial.println(strAux2);
  if((comando != "") && (ret == false) && (strstr(strAux2,"+CREG: 0,3")!= NULL)){
    Serial1.println(comando);
    memset(strAux2,'/0',250);
  }
  //Serial.println(strAux2);
  //Serial.println(strAux2);
  return ret;
}


uint8_t sendATcommandForDNS(char* respuestaEsperada){
  /*!
   * Envia el comando al modem que dado un dns devuelve la ip.
   * Esta hecho a huevo siguiendo el protocolo del modem
   * Con la respuesta esperado corroboro que la ip devuelta es correcta.
   */
   
  // el caracter aschii de "+" es 43
  uint8_t   j=0, answer =0;
  while((Serial1.available() > 0)){ 
   // Serial.println("Entre a DNS");
    strAux2[iATcommand] = (char)Serial1.read();    
    iATcommand++;
    
    if ((strstr(strAux2, respuestaEsperada) != NULL))  // Me fijo si encontre el caracter esperado. me devulve un puntero a la primer ocurrencia. Espero el + 
      {
        //Serial.println("puntero");
        ptch = strAux2 +strlen(strAux2) ;
        Serial.println(strAux2);
        while ((strcmp(ptch-1,"\n")!=0) && (answer ==0) && (millis() - timestamp < 3*timeOutModem) ) { // quedo apuntando en la fila del +DNS:ip yo me quiereo quedar con el ip.
         Serial.println("0000000");
         if(Serial1.available()!=0){
            strAux2[iATcommand] = (char)Serial1.read();          
            ipServer[j] = *ptch;  // solo me importa a partir de +DNS:
            j++;
            iATcommand++;
            ptch++; 
            Serial.println(ipServer);  
       //delay(100);     
          }
        } 
        if (millis() - timestamp < 3*timeOutModem){
          //ipServer[j-1] = '\0'; // borro el ultimo caracter que seria el \n
          ipServer[j-2] = '\0'; // borro el ultimo caracter que seria el \n
          //*OK_ERROR_MODEM = MESSAGE_RECIVED_TCP;
          answer = 1;
          //Serial.print("Coincidio primera respuesta");
          //Serial.println();
          break;
        }else{
          //*OK_ERROR_MODEM = ERROR_MODEM; //cambiar
          answer = 0;
        }
      }
  }
   //Serial.print("aeeeeeer");
  return answer;
}

uint8_t reciveFromServer(char* respuestaEsperada){
  /*!
   * Funcion que procesa la respuesta del servidor a un mensaje
   * Funciona usando el protocolo interno : cada mensaje empeiza con * y termina con #
   */
   
  // el caracter aschii de "+" es 43
  uint8_t answer = 0;
  while((Serial1.available() > 0)){ 
   // Serial.println("Entre a DNS");
    strAux2[iATcommand] = (char)Serial1.read();    
    iATcommand++;
    //Serial.println(strAux2);
    if ((strstr(strAux2, respuestaEsperada) != NULL))  // Me fijo si encontre el caracter esperado.   // me devulve un puntero a la primer ocurrencia. Espero el + 
      {
        //Serial.println("puntero");
        ptch = strAux2 +strlen(strAux2) ;
        //Serial.println(strAux2);
        while ((strcmp(ptch-1,"*")!=0) && (answer ==0) && (millis() - timestamp < 7*timeOutModem) ) { // quedo apuntando en la fila del +DNS:ip yo me quiereo quedar con el ip.
         //Serial.println("00");
         if(Serial1.available()!=0){
            strAux2[iATcommand] = (char)Serial1.read();          
            iATcommand++;
            ptch++;       
          }
        }
        do { 
          if(Serial1.available()!=0){
            strAux2[iATcommand] = (char)Serial1.read();
            *modemBufferIn = *ptch;  // solo me importa a partir de +DNS:          
            modemBufferIn++;
            iATcommand++;
            ptch++;       
          }
        } while ((strcmp(ptch-1,"#")!=0) && (answer ==0) && (millis() - timestamp < 7*timeOutModem) );
        if (millis() - timestamp < 7*timeOutModem){
          answer = 1;
          *(modemBufferIn -1) = '\0';
          Serial.println();
          //Serial.print("Mensaje Recivdo desde el Servidor: ");
          //Serial.println(strAux2);
          Serial.print(modemBufferIn);
          Serial.println();
          //*OK_ERROR_MODEM = MESSAGE_RECIVED_TCP;
          //*OK_ERROR_MODEM = OK_MODEM; //agregar esto una vez que hable con el mac
          break;
        }else{
          //OK_ERROR_MODEM = ERROR_MODEM; //cambiar
          answer = 0;
          Serial.print("NOOOOOO Coincidio primera respuesta");
          Serial.println(strAux2);
        }
      }
  }
  return answer;
}

int _initModem(struct pt *pt){ //le borre el static al ppio
  /*!
   * Configuro Baudrate
   * Apago ECHO. 
   * Espero a estar registrado en la Red
   * Protcolo Interno
   * Configuro APN 
   * Configuro Contraseñas
   * Configuro PPP
   * Le doy tiempo a que me den una IP
   */

  PT_BEGIN(pt);
    while(ESTADO_MODEM == INIT_MODEM){
      //Serial1.begin(9600);
      *OK_ERROR_MODEM = ERROR_MODEM;
      resetVariables();
      Serial1.println("ATE0");
      PT_WAIT_UNTIL(pt, (millis()-timestamp > timeOutModem) || checkResponseATcommand("","OK"));
      if(millis()-timestamp>timeOutModem)  break;
      resetVariables();
      Serial1.println("AT+CREG?");
      PT_WAIT_UNTIL(pt, (millis()-timestamp > 16*timeOutModem) || checkResponseATcommand("AT+CREG?","+CREG: 0,1"));
      if(millis()-timestamp > 16*timeOutModem)  break;
      resetVariables();
      Serial1.println("AT+XISP=0");  
      PT_WAIT_UNTIL(pt, (millis()-timestamp > 3*timeOutModem) || checkResponseATcommand("","OK"));
      if(millis()-timestamp>3*timeOutModem) break;
      resetVariables(); 
      Serial1.println("AT+CGDCONT=1,\"IP\",\"prepago.ancel\"");
      PT_WAIT_UNTIL(pt, millis()-timestamp > 3*timeOutModem || checkResponseATcommand("","OK")); 
      if(millis()-timestamp>3*timeOutModem) break;
      resetVariables();
      Serial1.println("AT+XGAUTH=1,1,\"BAM\",\"BAM\"");
      PT_WAIT_UNTIL(pt, millis()-timestamp > 2*timeOutModem || checkResponseATcommand("","OK")); 
      if(millis()-timestamp>2*timeOutModem) break;
      resetVariables();
      Serial1.println("AT+XIIC=1");
      PT_WAIT_UNTIL(pt, millis()-timestamp > 2*timeOutModem || checkResponseATcommand("","OK"));
      if(millis()-timestamp>2*timeOutModem) break;
      resetVariables();
      PT_WAIT_UNTIL(pt, millis()-timestamp > 10*timeOutModem); 
      Serial.println("END");
      ESTADO_MODEM = NO_TASK_MODEM;
      *OK_ERROR_MODEM = OK_MODEM;
    }
 PT_END(pt);

}

 int _enviarDatosTCP(char* TCPBufferOut,char* dns, struct pt *pt){
  /*!
   * 1) Busco la IP del servidor con el dns 
   * 2) establesco conexion. 
   * 3) aviso cuantos datos voy a mandar.
   * 4) mando los datos. 
   * 5) cierro la conexion.
   */
  //ESTADO_MODEM = SEND_TCP_MODEM;
  
  PT_BEGIN(pt);
 // timeOutModem = 1000;
 // puerto = 28564;
  //Serial.println(ptch);
  while(ESTADO_MODEM == SEND_TCP_MODEM){
    //*OK_ERROR_MODEM = ERROR_MODEM;
    Serial.println("Entre a SEND_TCP_MODEM");
    resetVariables();
    memset(ipServer,'\0',20); // borro el bufferIp antes de empezar.
    strcpy(strAux,"AT+DNS=\"");
    strcat(strAux,pDns);
    strcat(strAux,"\"");
    Serial1.println(strAux); 
    PT_WAIT_UNTIL(pt, sendATcommandForDNS("+DNS:") || millis() - timestamp > 3*timeOutModem ); // espero la respuesta +DNS=IP
    if(millis()-timestamp > 3*timeOutModem) break;
    resetVariables();
    strcpy(strAux,"AT+TCPSETUP=0,");
    strcat(strAux,ipServer);
    strcat(strAux,",");
    sprintf(s,"%d",puerto);
    strcat(strAux,s);
    Serial1.println(strAux);
    PT_WAIT_UNTIL(pt, (millis()-timestamp > 5*timeOutModem) || checkResponseATcommand("","+TCPSETUP:0,OK")); 
    if(millis()-timestamp > 5*timeOutModem) {
      Serial1.println("AT+TCPCLOSE=0"); //!< cierro conexion por las dudas que halla quedado abierta de antes.
      break;
    }
    resetVariables();
    sprintf(strAux,"AT+TCPSEND=0,%d",strlen(ptBufferTcpOut));
    Serial.println(strAux);
    Serial1.println(strAux);
    PT_WAIT_UNTIL(pt, (millis()-timestamp > timeOutModem));
    resetVariables();
    Serial1.println(ptBufferTcpOut);
    PT_WAIT_UNTIL(pt, (millis()-timestamp > 10*timeOutModem) || reciveFromServer("+TCP"));
    if(millis()-timestamp > 10*timeOutModem) break;
    resetVariables();
    Serial1.println("AT+TCPCLOSE=0");
    //(*OK_ERROR_MODEM = MESSAGE_RECIVED_TCP) ? *OK_ERROR_MODEM = MESSAGE_RECIVED_TCP: *OK_ERROR_MODEM = OK_MODEM;
    PT_WAIT_UNTIL(pt, (millis()-timestamp > 2*timeOutModem) || checkResponseATcommand("","+TCPCLOSE:0"));
    *OK_ERROR_MODEM = MESSAGE_RECIVED_TCP;
    ESTADO_MODEM = NO_TASK_MODEM;
    resetVariables();
  }//
  PT_END(pt);
}

void enviarDatosTCP(char* TCPBufferOut, char* dns, char *mensajeTcpRecivido){
  /*!
   * Funcion Publica
   * Asigno el contenido de los punteros que vienen del main al los punteros internos del Modem.cpp  
   * cargo en el puntero error el resultado de las funciones. 
   */
  //PT_INIT(&pt1);  // initialise the two
  //PT_INIT(&pt2);  // protothread variables
  ESTADO_MODEM = SEND_TCP_MODEM;
  ptBufferTcpOut = TCPBufferOut;
  pDns = dns;
  modemBufferIn = mensajeTcpRecivido;
  //mensajeTcpRecivido = modemBufferIn;
   //OK_ERROR_MODEM = errorModem;
  *OK_ERROR_MODEM = ERROR_MODEM;
}

void initModem(uint16_t unPuerto, uint16_t untimeout,MODEM_STATE* errorModem){
  /*!
   * Funcion Publica
   * Asigno el contenido de los punteros que vienen del main al los punteros internos del Modem.cpp  
   * cargo en el puntero error el resultado de las funciones. 
   */ 
  PT_INIT(&pt1);  // initialise the two
  PT_INIT(&pt2);  // protothread variables
  PT_INIT(&pt3);
  PT_INIT(&pt4);  // protothread variables
  ESTADO_MODEM = INIT_MODEM;
  puerto = unPuerto;
  timeOutModem = untimeout; 
  OK_ERROR_MODEM = errorModem;
  *OK_ERROR_MODEM = ERROR_MODEM;
}

void sendFtpFile(char* dns, char* file, uint16_t largoFile, uint16_t port, char* user, char* pwd){

  ESTADO_MODEM = SEND_FTP_FILE;
  pFile = file;
  largoFile = largoFile;
  puertoFtp = port;
  pUser = user;
  pPwd = pwd;
  pDns = dns;
  *OK_ERROR_MODEM = ERROR_MODEM;

}


void resetVariables(){
 /*!
  * Esta funcion es llamada internamente por funcinoes internas del modem
  * resetea valores de algunas variables globales 
  */
  memset(strAux2,'\0', 250);
  memset(strAux,'\0', 50);
  while(Serial1.available() > 0) Serial1.read();
  iATcommand=0;
  timestamp = millis();
  //answer = 0;
}

void resetearModem(){
  Serial1.println("AT+TCPCLOSE=0"); //cierro puerto TCO
  delay(100);
  Serial1.println("AT+FTPLOGOUT"); // cieroo puerto FTP
  *OK_ERROR_MODEM = OK_MODEM;
  ESTADO_MODEM = INIT_MODEM; //reseteo el modem.
}

void clearErrorModem(MODEM_STATE* errorModem){
  *errorModem = OK_MODEM;
}

void getIp(char* ip, char* miDns){
  resetVariables();
  memset(ipServer,'\0',20); // borro el bufferIp antes de empezar.
  strcpy(strAux,"AT+DNS=\"");
  strcat(strAux,miDns);
  strcat(strAux,"\"");
  sendATcommandForDNS("+DNS:");
  strcpy(ip,ipServer); 
}

int _sendFtpFile(struct pt *pt){
  //AT+FTPLOGIN=<ip>,<port>,<user>,<pwd>
  PT_BEGIN(pt);
    *OK_ERROR_MODEM = ERROR_MODEM;
    Serial.println("*********Entre a send FTP file");
    resetVariables();
    memset(ipServer,'\0',20); // borro el bufferIp antes de empezar.
    strcpy(strAux,"AT+DNS=\"");
    strcat(strAux,pDns);
    strcat(strAux,"\"");
    Serial1.println(strAux); 
    PT_WAIT_UNTIL(pt, sendATcommandForDNS("+DNS:") || millis() - timestamp > 5*timeOutModem ); // espero la respuesta +DNS=IP
    if(millis()-timestamp > 5*timeOutModem) break;
    resetVariables();
    strcpy(strAux,"AT+FTPLOGIN=");
    strcat(strAux,ipServer);
    //strcat(strAux,"\",\"");
    sprintf(s,",%d,",puertoFtp);
    strcat(strAux,s);
    strcat(strAux,pUser);
    strcat(strAux,",");
    strcat(strAux,pPwd);
    //strcat(strAux,"");
    Serial1.println(strAux);
    Serial.print("****mande el login con: ");
    Serial.println(strAux);
    PT_WAIT_UNTIL(pt, (millis()-timestamp > 6*timeOutModem) || checkResponseATcommand("","+FTPLOGIN:User logged in"));
    if(millis()-timestamp > 6*timeOutModem) {  
        Serial1.println("AT+FTPLOGOUT");
        Serial.println("mande al logout"); 
        break;
    }
    //AT+FTPPUT=<filename>,<type>,<mode>,<size>
    resetVariables();
    Serial.println("****mande los archivos");
    strcpy(strAux,"AT+FTPPUT=");
    strcat(strAux,pFile);
    strcat(strAux,",2,1,");
    sprintf(s,"%d",largoFile);
    strcat(strAux,s);
    Serial1.println(strAux);
    Serial.println("Me logee*****");
    PT_WAIT_UNTIL(pt, (millis()-timestamp > timeOutModem));
    //escribir archivo conntenido en SD.
    sendSDfileToserver(pFile);
    PT_WAIT_UNTIL(pt, (millis()-timestamp > 6*timeOutModem) || checkResponseATcommand("","+FTPPUT :OK"));
    if(millis()-timestamp > 6*timeOutModem) break;
    //Logout
    Serial1.println("AT+FTPLOGOUT");
    resetVariables();
    PT_WAIT_UNTIL(pt, (millis()-timestamp > 3*timeOutModem) || checkResponseATcommand("","OK"));
    //if(millis()-timestamp > 3*timeOutModem) break;
    *OK_ERROR_MODEM = OK_MODEM;
    ESTADO_MODEM = NO_TASK_MODEM;
  PT_END(pt);
}

int _enviarSms(struct pt *pt ){
  PT_BEGIN(pt);
  while(ESTADO_MODEM == SEND_SMS_MODEM){
    resetVariables();
    Serial1.println("AT+CMGF=1"); // AT command to send SMS message
    PT_WAIT_UNTIL(pt, (millis()-timestamp > timeOutModem) || checkResponseATcommand("","OK"));
    if(millis()-timestamp>timeOutModem)  break;
    resetVariables();
    Serial1.println("AT+CNMI=2,2,0,0,0"); // config pa que mande los sms que reciva por uart
    PT_WAIT_UNTIL(pt, (millis()-timestamp > timeOutModem) || checkResponseATcommand("","OK"));
    if(millis()-timestamp>timeOutModem)  break;
    resetVariables();
    Serial1.println("AT+CSCS=\"GSM\""); 
    PT_WAIT_UNTIL(pt, (millis()-timestamp > timeOutModem) || checkResponseATcommand("","OK"));
    resetVariables();
    strcpy(strAux,"AT+CMGS = \"");
    strcat(strAux,pNumeroTel);
    strcat(strAux,"\"");
    Serial1.println(strAux);
    Serial.print("numero: ");
    Serial.println(strAux); 
    PT_WAIT_UNTIL(pt, (millis()-timestamp > timeOutModem));
    //resetVariables();
    Serial1.print(pMensajeSms);  // message to send
    Serial.print("mensaje: ");
    Serial.println(pMensajeSms);
    PT_WAIT_UNTIL(pt, (millis()-timestamp > timeOutModem));
    Serial1.println((char)26); // caracter para indicar que termino el mensaje a enviar
    ESTADO_MODEM = NO_TASK_MODEM;
    *OK_ERROR_MODEM = OK_MODEM;
    Serial.println("fin envio SMS");
  }
  PT_END(pt);

}

void enviarSms(char* mensaje, char* numero){
  ESTADO_MODEM = SEND_SMS_MODEM;
  pMensajeSms = mensaje;
  pNumeroTel = numero;

  *OK_ERROR_MODEM = ERROR_MODEM;
  //*_OK_ERROR_MODEM = ERROR_MODEM;
}
void modemTask(){
  /*! 
   * Funcion Publica
   * Esta funcion la llama el main periodicamente. Cada vez que la llaman ejecuta un paso de la maqiuna de estados del modem.  
   */
   //Serial.println(ESTADO_MODEM);
   //Serial.println("TM");
   switch(ESTADO_MODEM){
    case INIT_MODEM:
      _initModem(&pt1);
      break;
    
    case SEND_TCP_MODEM:
      _enviarDatosTCP(ptch,pDns,&pt2);
      break;
    
    case SEND_FTP_FILE:
      _sendFtpFile(&pt4);
      break;
      
    case SEND_SMS_MODEM:
      _enviarSms(&pt3);
      break;
    
    case NO_TASK_MODEM:
    break;
    default: break;

   }

}





