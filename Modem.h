#ifndef MODEM_H
#define MODEM_H

#include "Arduino.h"
#include "pt.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <avr/io.h>
#include <HardwareSerial.h>
#include <stdint.h>

typedef enum MODEM_STATE {OK_MODEM,ERROR_MODEM,SEND_SMS_MODEM,INIT_MODEM, SEND_TCP_MODEM, SEND_FTP_FILE, MESSAGE_RECIVED_TCP, RESET_MODEM, NO_TASK_MODEM};

void initModem(uint16_t unPuerto, uint16_t untimeout, MODEM_STATE* errorModem);
void enviarDatosTCP(char* TCPBufferOut, char* dns, char *mensajeTcpRecivido, bool respuesta);
void sendFtpFile(char* dns,char* file, uint16_t largoFile, uint16_t port, char* user, char* pwd);
void recibirDatosTCP(char* TCPBufferIn);
void enviarSms(char* mensaje, char* numero);
void resetearModem();
void modemTask();

#endif
