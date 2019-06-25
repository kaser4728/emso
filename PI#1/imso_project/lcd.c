#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <mosquitto.h>

#define mqtt_host "192.168.0.12"
#define mqtt_port 1883
#define MQTT_TOPIC "myTopic"

#define I2C_BUS        "/dev/i2c-1" // I2C bus device on a Raspberry Pi 3
#define I2C_ADDR       0x27         // I2C slave address for the LCD module
#define BINARY_FORMAT  " %c  %c  %c  %c  %c  %c  %c  %c\n"
#define BYTE_TO_BINARY(byte) \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

#define I2C_CLEAR_LCD()		\
   usleep(40);			\
   i2c_send_byte(0b00000100);	\
   i2c_send_byte(0b00000000);	\
   i2c_send_byte(0b00010100);	\
   i2c_send_byte(0b00010000);

/*
 * 2nd row
 *
   usleep(40);
   i2c_send_byte(0b11000100); // DDRAM AD SET
   i2c_send_byte(0b11000000); // send 1(D7)/100
   i2c_send_byte(0b00000100); // DDRAM AD SET
   i2c_send_byte(0b00000000); // send 0000, ADD=100 0000=0x40=2nd row, 1st column addr
*/

#define I2C_ROW_1ST()		\
   usleep(40);			\
   i2c_send_byte(0b10000100);	\
   i2c_send_byte(0b10000000);	\
   i2c_send_byte(0b00000100);	\
   i2c_send_byte(0b00000000); 

#define I2C_ROW_2ND()		\
   usleep(40);			\
   i2c_send_byte(0b11000100);	\
   i2c_send_byte(0b11000000);	\
   i2c_send_byte(0b00000100);	\
   i2c_send_byte(0b00000000); 

#define I2C_ROW_3RD()		\
   usleep(40);			\
   i2c_send_byte(0b10010100);	\
   i2c_send_byte(0b10010000);	\
   i2c_send_byte(0b01000100);	\
   i2c_send_byte(0b01000000); 

#define I2C_ROW_4TH()		\
   usleep(40);			\
   i2c_send_byte(0b11010100);	\
   i2c_send_byte(0b11010000);	\
   i2c_send_byte(0b01000100);	\
   i2c_send_byte(0b01000000); 

static int run = 1;

int lcd_backlight;
int debug;
char address; 
int i2cFile;

void i2c_start() {
   if((i2cFile = open(I2C_BUS, O_RDWR)) < 0) {
      printf("Error failed to open I2C bus [%s].\n", I2C_BUS);
      exit(-1);
   }
   // set the I2C slave address for all subsequent I2C device transfers
   if (ioctl(i2cFile, I2C_SLAVE, I2C_ADDR) < 0) {
      printf("Error failed to set I2C address [%s].\n", I2C_ADDR);
      exit(-1);
   }
}

void i2c_stop() { close(i2cFile); }

void i2c_send_byte(unsigned char data) {
   unsigned char byte[1];
   byte[0] = data;
   if(debug) printf(BINARY_FORMAT, BYTE_TO_BINARY(byte[0]));
   write(i2cFile, byte, sizeof(byte)); 
   /* -------------------------------------------------------------------- *
    * Below wait creates 1msec delay, needed by display to catch commands  *
    * -------------------------------------------------------------------- */
   usleep(1000);
}

void i2c_send_char(const unsigned char ch){
	const unsigned char chMaskTen = 0b11110000;
	const unsigned char chMaskOne = 0b00001111;
	unsigned char t_chTen, t_chOne;

	t_chTen = ch & chMaskTen;
	t_chOne = ch & chMaskOne;

	t_chOne = t_chOne << 4;

	i2c_send_byte(t_chTen + 0b00001101);
	i2c_send_byte(t_chTen + 0b00001001);
	i2c_send_byte(t_chOne + 0b00001101);
	i2c_send_byte(t_chOne + 0b00001001);
}

void i2c_send_str(const char str[]){
	int i = 0;
	int strSize = strlen(str);

	for(i = 0;i < strSize;i++){
		i2c_send_char(str[i]);
	}
}

unsigned int count_digit(unsigned int n){
	unsigned int i = 1;

	while(n /= 10){
		i++;
	}

	return i;
}

void i2c_send_int(const int n){
	int i = 0;
	unsigned int tN = (n >= 0) ? n : 0;
	const unsigned int digits = count_digit(tN);
	unsigned char n_arr[digits];

	for(i = digits-1;i >= 0;i--){
		n_arr[i] = tN % 10;
		tN /= 10;
	}

	for(i = 0;i < digits;i++){
		i2c_send_char((unsigned char)('0' + n_arr[i]));
	}
}

void handle_signal(int s){
	run = 0;
}

void connect_callback(struct mosquitto *mosq, void *obj, int result){
	printf("connect callback, rc=%d\n", result);
}

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message){
	bool match = 0;

	//printf("received message(%s): %s", message->topic, message->payload);


  
   I2C_ROW_2ND();
   i2c_send_str(message->payload);

	if(match){
		printf("got message for ADC topic\n");
	}
}

void main(int argc, char *argv[]) { 
	int rc = 0;
	uint8_t reconnect = true;
	struct mosquitto *mosq;

   i2c_start(); 
   debug=0;//1

   /* -------------------------------------------------------------------- *
    * Initialize the display, using the 4-bit mode initialization sequence *
    * -------------------------------------------------------------------- */
   if(debug) printf("Init Start:\n");
   if(debug) printf("D7 D6 D5 D4 BL EN RW RS\n");

   usleep(15000);             // wait 15msec
   i2c_send_byte(0b00110100); // D7=0, D6=0, D5=1, D4=1, RS,RW=0 EN=1
   i2c_send_byte(0b00110000); // D7=0, D6=0, D5=1, D4=1, RS,RW=0 EN=0
   usleep(4100);              // wait 4.1msec
   i2c_send_byte(0b00110100); // 
   i2c_send_byte(0b00110000); // same
   usleep(100);               // wait 100usec
   i2c_send_byte(0b00110100); //
   i2c_send_byte(0b00110000); // 8-bit mode init complete
   usleep(4100);              // wait 4.1msec
   i2c_send_byte(0b00100100); //
   i2c_send_byte(0b00100000); // switched now to 4-bit mode

   /* -------------------------------------------------------------------- *
    * 4-bit mode initialization complete. Now configuring the function set *
    * -------------------------------------------------------------------- */
   usleep(40);                // wait 40usec
   i2c_send_byte(0b00100100); //
   i2c_send_byte(0b00100000); // keep 4-bit mode
   i2c_send_byte(0b10000100); //
   i2c_send_byte(0b10000000); // D3=2lines, D2=char5x8

   /* -------------------------------------------------------------------- *
    * Next turn display off                                                *
    * -------------------------------------------------------------------- */
   usleep(40);                // wait 40usec
   i2c_send_byte(0b00000100); //
   i2c_send_byte(0b00000000); // D7-D4=0
   i2c_send_byte(0b10000100); //
   i2c_send_byte(0b10000000); // D3=1 D2=display_off, D1=cursor_off, D0=cursor_blink

   /* -------------------------------------------------------------------- *
    * Display clear, cursor home                                           *
    * -------------------------------------------------------------------- */
   usleep(40);                // wait 40usec
   i2c_send_byte(0b00000100); //
   i2c_send_byte(0b00000000); // D7-D4=0
   i2c_send_byte(0b00010100); //
   i2c_send_byte(0b00010000); // D0=display_clear

   /* -------------------------------------------------------------------- *
    * Set cursor direction                                                 *
    * -------------------------------------------------------------------- */
   usleep(40);                // wait 40usec
   i2c_send_byte(0b00000100); //
   i2c_send_byte(0b00000000); // D7-D4=0
   i2c_send_byte(0b01100100); //
   i2c_send_byte(0b01100000); // print left to right

   /* -------------------------------------------------------------------- *
    * Turn on the display                                                  *
    * -------------------------------------------------------------------- */
   usleep(40);                // wait 40usec
   i2c_send_byte(0b00000100); //
   i2c_send_byte(0b00000000); // D7-D4=0
   i2c_send_byte(0b11100100); //
   i2c_send_byte(0b11100000); // D3=1 D2=display_on, D1=cursor_on, D0=cursor_blink

   if(debug) printf("Init End.\n");
   //sleep(1);
   
   if(debug) printf("Writing HELLO to display\n");
   if(debug) printf("D7 D6 D5 D4 BL EN RW RS\n");

   /* -------------------------------------------------------------------- *
    * Start writing 'H' 'E' 'L' 'L' 'O' chars to the display, with BL=on.  *
    * -------------------------------------------------------------------- */
/*
   i2c_send_byte(0b01001101); //
   i2c_send_byte(0b01001001); // send 0100=4
   i2c_send_byte(0b10001101); //
   i2c_send_byte(0b10001001); // send 1000=8 = 0x48 ='H'

   i2c_send_byte(0b01001101); //
   i2c_send_byte(0b01001001); // send 0100=4
   i2c_send_byte(0b01011101); // 
   i2c_send_byte(0b01011001); // send 0101=1 = 0x41 ='E'

   i2c_send_byte(0b01001101); //
   i2c_send_byte(0b01001001); // send 0100=4
   i2c_send_byte(0b11001101); //
   i2c_send_byte(0b11001001); // send 1100=12 = 0x4D ='L'

   i2c_send_byte(0b01001101); //
   i2c_send_byte(0b01001001); // send 0100=4
   i2c_send_byte(0b11001101); //
   i2c_send_byte(0b11001001); // send 1100=12 = 0x4D ='L'

   i2c_send_byte(0b01001101); //
   i2c_send_byte(0b01001001); // send 0100=4
   i2c_send_byte(0b11111101); //
   i2c_send_byte(0b11111001); // send 1111=15 = 0x4F ='O'
   */

	 I2C_CLEAR_LCD();
   	i2c_send_str("REMAINING: ");
	I2C_ROW_2ND();
   	i2c_send_str("[0/0] SLOTS");

	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	mosquitto_lib_init();

	mosq = mosquitto_new(NULL, true, 0);

	if(mosq){
		mosquitto_connect_callback_set(mosq, connect_callback);
		mosquitto_message_callback_set(mosq, message_callback);

		rc = mosquitto_connect(mosq, mqtt_host, mqtt_port, 60);

		mosquitto_subscribe(mosq, NULL, "myTopic", 0);

		while(run){
			rc = mosquitto_loop(mosq, -1, 1);

			if(run && rc){
				printf("connection eror!\n");
				sleep(10);
				mosquitto_reconnect(mosq);
			}
		}
		mosquitto_destroy(mosq);
	}

	mosquitto_lib_cleanup();






	/*
   i2c_send_str("REMAINING: ");
   I2C_ROW_2ND();
   i2c_send_str("   ");

   i2c_send_byte(0b10101101); //
   i2c_send_byte(0b10101001); // send 1010
   i2c_send_byte(0b00101101); //
   i2c_send_byte(0b00101001); // send 0010 = '['

   i2c_send_int(atoi(argv[1]));

   i2c_send_byte(0b10101101); //
   i2c_send_byte(0b10101001); // send 1010
   i2c_send_byte(0b00111101); //
   i2c_send_byte(0b00111001); // send 0011 = ']'

   i2c_send_str(" CARS");






   sleep(3);
   I2C_CLEAR_LCD();
   i2c_send_str("*LCD CLEAR TEST*");
   I2C_ROW_2ND();
   i2c_send_int(12);
   i2c_send_str("@$#%");
   i2c_send_int(34);
   i2c_send_str(">? ");

   i2c_send_byte(0b10111101);
   i2c_send_byte(0b10111001);
   i2c_send_byte(0b00111101);
   i2c_send_byte(0b00111001);

   i2c_send_byte(0b11001101);
   i2c_send_byte(0b11001001);
   i2c_send_byte(0b10111101);
   i2c_send_byte(0b10111001);

   i2c_send_byte(0b10101101);
   i2c_send_byte(0b10101001);
   i2c_send_byte(0b11101101);
   i2c_send_byte(0b11101001);
   i2c_send_char('-');
   i2c_send_str(" ^_^ ");

   i2c_send_byte(0b11111101);
   i2c_send_byte(0b11111001);
   i2c_send_byte(0b11001101);
   i2c_send_byte(0b11001001);
   */






   if(debug) printf("Finished writing to display.\n");
   i2c_stop(); 
}
