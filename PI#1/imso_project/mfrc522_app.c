#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>

#include "mfrc522_lib.h"
#include "pipark.h"
#include "mosquitto.h"

uint8_t mode = 0;
uint8_t bits = 8;
uint32_t speed = 4000000;

#define mqtt_host "localhost"
#define mqtt_port 1883
#define mqtt_keepalive 60

Uid uid;

const char auth[10] = {0xB3, 0x8D, 0xE9, 0x53, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

char comp[10] = { 0 };

void spi_init(void) {

  spi_dev = open("/dev/spidev0.0", O_RDWR);
  if (spi_dev < 0) {
    printf("Opening spidev failed. Run with sudo\n");
    exit(-1);
  }
  pipark_dev = open("/dev/pipark", O_RDWR);
  if (pipark_dev < 0) {
    printf("opening pipark failed. Run mknod\n");
    exit(-1);
  }

  int ret;
  ret = ioctl(spi_dev, SPI_IOC_WR_MODE, &mode);
  if (ret < 0) { 
    printf("Can't set spi write mode");
    exit(-1);
  }
  ret = ioctl(spi_dev, SPI_IOC_RD_MODE, &mode);
  if (ret < 0) { 
    printf("Can't set spi read mode");
    exit(-1);
  } 
  ret = ioctl(spi_dev, SPI_IOC_WR_BITS_PER_WORD, &bits);
  if (ret < 0) { 
    printf("Can't set spi bits per word");
    exit(-1);
  }
  ret = ioctl(spi_dev, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret < 0) { 
    printf("Can't get spi bits per word");
    exit(-1);
  }
  ret = ioctl(spi_dev, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
  if (ret < 0) { 
    printf("Can't set spi max speed"); 
    exit(-1);
  }
  ret = ioctl(spi_dev, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
  if (ret < 0) { 
    printf("Can't get spi max speed"); 
    exit(-1);
  }

}
/* 
void auth_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	if(message->payloadlen){
		if (message->payload == "authorized") {
      ioctl(pipark_dev, AUTHORIZED, NULL);
    }
    else {
      ioctl(pipark_dev, UNAUTHORIZED, NULL);
    }
	}
} */

int main(void) {

  spi_init();
  pcd_init();


  while(1){
    if(!picc_isNewCardPresent()) {
      usleep(5000);
      continue;
    }    
    if(!picc_readCardSerial(&uid)) {
      usleep(5000);
      continue;
    }

    /* if (*uid.uidByte != *comp) {
      memcpy(comp, uid.uidByte, 10);
    }
    else {
      continue;
    } */

    printf("Card Detected! Serial: \n");
    for(byte i = 0; i < uid.size; ++i) {
        printf("%.2X ",uid.uidByte[i]);
    }
    printf("\n");
    ioctl(pipark_dev, WRITE_UID, &uid.uidByte);

    /* if (*uid.uidByte == *auth) {
      ioctl(pipark_dev, AUTHORIZED, NULL);
    }
    else {
      ioctl(pipark_dev, UNAUTHORIZED, NULL);
    }*/
  }
    

  return 0;
}