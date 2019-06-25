#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "mfrc522_lib.h"
#include "pipark.h"

#define RSTPIN 5

//#define debug
#ifdef debug
  #define PDEBUG(fmt, args...) printf(fmt, ##args)
#else
  #define PDEBUG(fmt, args...)
#endif


void pcd_writeRegister(byte reg, byte value) {

  byte data[2];
  data[0] = reg & 0x7E;
  data[1] = value;

  struct spi_ioc_transfer tr = {
    .tx_buf = (unsigned long)&data,
    .rx_buf = (unsigned long)NULL,
    .len = 2
  };

  int ret = ioctl(spi_dev, SPI_IOC_MESSAGE(1), &tr);
  if (ret < 1) {
    printf("Can't send spi message\n");
    return;
  }

}

void pcd_writeRegisterN(byte reg, byte count,	byte *values) {
  byte index;
  for (index = 0; index < count; index++) {
  	pcd_writeRegister(reg, values[index]);
	}

}

byte pcd_readRegister(byte reg) {
  
  byte send_data[2];
  send_data[0] = 0x80 | ((reg) & 0x7E);
  send_data[1] = 0;

  byte recv_data[2];

  struct spi_ioc_transfer tr = {
    .tx_buf = (unsigned long)&send_data,
    .rx_buf = (unsigned long)&recv_data,
    .len = 2
  };

  int ret = ioctl(spi_dev, SPI_IOC_MESSAGE(1), &tr);
  PDEBUG("Read register value : %.2x\n", recv_data[1]);
  if (ret < 1) {
    printf("Can't send spi message\n");
    return 0;
  }

  return recv_data[1]; 
}

void pcd_readRegisterN(	byte reg,	byte count,	byte *values,	byte rxAlign) {

  if (count == 0) {
    return;
  }

  byte address = 0x80 | (reg & 0x7E);
  byte index = 0;
  count--;
  struct spi_ioc_transfer tr = {
    .tx_buf = (unsigned long)&address,
    .rx_buf = (unsigned long)NULL,
    .len = 1
  };
  int ret = ioctl(spi_dev, SPI_IOC_MESSAGE(1), &tr);
  if (ret < 1) {
    printf("Can't send spi message\n");
    return;
  }

  while (index < count) {
    if (index == 0 && rxAlign) {
      byte mask = 0;
      byte i;
      for (i = rxAlign; i <= 7; i++) {
	      mask |= (1 << i);
      }
      byte value;
      struct spi_ioc_transfer tr2 = {
        .tx_buf = (unsigned long)&address,
        .rx_buf = (unsigned long)&value,
        .len = 1
      };
      ret = ioctl(spi_dev, SPI_IOC_MESSAGE(1), &tr2);
      PDEBUG("Read register value : %x\n", value);
      if (ret < 1) {
        printf("Can't send spi message\n");
        return;
      }
      
      values[0] = (values[index] & ~mask) | (value & mask);
    }
    else {
      struct spi_ioc_transfer tr2 = {
        .tx_buf = (unsigned long)&address,
        .rx_buf = (unsigned long)&values[index],
        .len = 1
      };
      ret = ioctl(spi_dev, SPI_IOC_MESSAGE(1), &tr2);
      PDEBUG("Read register value : %x\n", values[index]);
        if (ret < 1) {
        printf("Can't send spi message\n");
        return;
      }
      
    }
    index++;
  }
  byte zero = 0;
  struct spi_ioc_transfer tr2 = {
        .tx_buf = (unsigned long)&zero,
        .rx_buf = (unsigned long)&values[index],
        .len = 1
      };
  ret = ioctl(spi_dev, SPI_IOC_MESSAGE(1), &tr2);
  PDEBUG("Read register value : %x\n", values[index]);
  if (ret < 1) {
    printf("Can't send spi message\n");
    return;
  }

}

void pcd_setRegisterBitMask(byte reg, byte mask) { 

  byte tmp;
  tmp = pcd_readRegister(reg);
  pcd_writeRegister(reg, tmp | mask);

}

void pcd_clearRegisterBitMask(byte reg, byte mask) {

  byte tmp;
  tmp = pcd_readRegister(reg);
  pcd_writeRegister(reg, tmp & (~mask));

}



void pcd_reset() {
  PDEBUG("Inside function pcd_reset\n");

  pcd_writeRegister(CommandReg, PCD_SoftReset);

  byte count = 0;
  do {
		usleep(50);
	} while ((pcd_readRegister(CommandReg) & (1 << 4)) && (++count) < 3);

} // End pcd_reset()


void pcd_antennaOn() {

  byte value = pcd_readRegister(TxControlReg);
  if ((value & 0x03) != 0x03) {
    pcd_writeRegister(TxControlReg, value | 0x03);
  }
} // End pcd_antennaOn()

void pcd_antennaOff() {
  pcd_clearRegisterBitMask(TxControlReg, 0x03);
} // End pcd_antennaOff()

void pcd_init() {
  PDEBUG("Inside function pcd_init\n");

  int rst_gpio = ioctl(pipark_dev, READ_GPIO, RSTPIN);
  if (rst_gpio == 0) {
    ioctl(pipark_dev, WRITE_GPIO_1, RSTPIN);
    usleep(50);
  } //hard reset
  else {
    pcd_reset();
  } //soft reset
	
  pcd_writeRegister(TModeReg, 0x80);
  pcd_writeRegister(TPrescalerReg, 0xA9);	
  pcd_writeRegister(TReloadRegH, 0x03);	
  pcd_writeRegister(TReloadRegL, 0xE8);
  pcd_writeRegister(TxASKReg, 0x40);	
  pcd_writeRegister(ModeReg, 0x3D);	
  pcd_antennaOn();

} // End pcd_init()


byte pcd_calculateCRC(byte *data,	byte length, byte *result) {

  pcd_writeRegister(CommandReg, PCD_Idle);
  pcd_writeRegister(DivIrqReg, 0x04);
  pcd_setRegisterBitMask(FIFOLevelReg, 0x80);
  pcd_writeRegisterN(FIFODataReg, length, data);
  pcd_writeRegister(CommandReg, PCD_CalcCRC);
	
  word i = 5000;
  byte n;
  while (1) {
    n = pcd_readRegister(DivIrqReg);
    if (n & 0x04) {
      break;
    }
    if (--i == 0) {
      return STATUS_TIMEOUT;
    }
  }
  pcd_writeRegister(CommandReg, PCD_Idle);
	
  result[0] = pcd_readRegister(CRCResultRegL);
  result[1] = pcd_readRegister(CRCResultRegH);

  return STATUS_OK;

} // End pcd_calculateCRC()

byte pcd_communicateWithPICC(byte command, byte waitIRq, byte *sendData, byte sendLen, byte *backData, byte *backLen, byte *validBits, byte rxAlign,	int checkCRC) {
  PDEBUG("Inside function pcd_communicateWithPICC\n");

  byte n, _validBits;
  unsigned int i;
	
  byte txLastBits = validBits ? *validBits : 0;
  byte bitFraming = (rxAlign << 4) + txLastBits;
	
  pcd_writeRegister(CommandReg, PCD_Idle);
  pcd_writeRegister(ComIrqReg, 0x7F);	
  pcd_setRegisterBitMask(FIFOLevelReg, 0x80);
  pcd_writeRegisterN(FIFODataReg, sendLen, sendData);	
  pcd_writeRegister(BitFramingReg, bitFraming);	
  pcd_writeRegister(CommandReg, command);
  if (command == PCD_Transceive) {
    pcd_setRegisterBitMask(BitFramingReg, 0x80);
  }
	
  i = 2000;
  while (1) {
    n = pcd_readRegister(ComIrqReg);
    if (n & waitIRq) {
      break;
    }
    if (n & 0x01) {	
      return STATUS_TIMEOUT;
    }
    if (--i == 0) {
      return STATUS_TIMEOUT;
    }
  }
	
  byte errorRegValue = pcd_readRegister(ErrorReg);
  if (errorRegValue & 0x13) {	
    return STATUS_ERROR;
  }	

  if (backData && backLen) {
    n = pcd_readRegister(FIFOLevelReg);
    if (n > *backLen) {
      return STATUS_NO_ROOM;
    }
    *backLen = n;		
    pcd_readRegisterN(FIFODataReg, n, backData, rxAlign);
    _validBits = pcd_readRegister(ControlReg) & 0x07;	
    if (validBits) {
      *validBits = _validBits;
    }
  }
	
  if (errorRegValue & 0x08) {	
    return STATUS_COLLISION;
  }
	
  if (backData && backLen && checkCRC) {
    if (*backLen == 1 && _validBits == 4) {
      return STATUS_MIFARE_NACK;
    }
      if (*backLen < 2 || _validBits != 0) {
      return STATUS_CRC_WRONG;
    }
    byte controlBuffer[2];
    n = pcd_calculateCRC(&backData[0], *backLen - 2, &controlBuffer[0]);
    if (n != STATUS_OK) {
      return n;
    }
    if ((backData[*backLen - 2] != controlBuffer[0]) || (backData[*backLen - 1] != controlBuffer[1])) {
      return STATUS_CRC_WRONG;
    }
  }
	
  return STATUS_OK;
} // End pcd_communicateWithPICC()

byte pcd_transceiveData(byte *sendData,	byte sendLen,	byte *backData,	byte *backLen, byte *validBits,	byte rxAlign,	int checkCRC) {
  PDEBUG("Inside function pcd_transceiveData\n");

  byte waitIRq = 0x30;
  return pcd_communicateWithPICC(PCD_Transceive, waitIRq, sendData, sendLen, backData, backLen, validBits, rxAlign, checkCRC);
} // End pcd_transceiveData()

byte picc_select(Uid *uid, byte validBits) {
  PDEBUG("Inside function picc_select\n\n");

  int uidComplete;
  int selectDone;
  int useCascadeTag;
  byte cascadeLevel = 1;
  byte result;
  byte count;
  byte index;
  byte uidIndex;				
  signed char currentLevelKnownBits;		
  byte buffer[9];					
  byte bufferUsed;				
  byte rxAlign;					
  byte txLastBits;			
  byte *responseBuffer;
  byte responseLength;
	
  if (validBits > 80) {
    return STATUS_INVALID;
  }
	
  pcd_clearRegisterBitMask(CollReg, 0x80);
	
  uidComplete = 0;
  while (!uidComplete) {
    switch (cascadeLevel) {
    case 1:
      buffer[0] = PICC_CMD_SEL_CL1;
      uidIndex = 0;
      useCascadeTag = validBits && uid->size > 4;	
      break;
			
    case 2:
      buffer[0] = PICC_CMD_SEL_CL2;
      uidIndex = 3;
      useCascadeTag = validBits && uid->size > 7;	
      break;
			
    case 3:
      buffer[0] = PICC_CMD_SEL_CL3;
      uidIndex = 6;
      useCascadeTag = 0;
      break;
			
    default:
      return STATUS_INTERNAL_ERROR;
      break;
    }
		
    currentLevelKnownBits = validBits - (8 * uidIndex);
    if (currentLevelKnownBits < 0) {
      currentLevelKnownBits = 0;
    }
    index = 2; 
    if (useCascadeTag) {
      buffer[index++] = PICC_CMD_CT;
    }
    byte bytesToCopy = currentLevelKnownBits / 8 + (currentLevelKnownBits % 8 ? 1 : 0);
    if (bytesToCopy) {
      byte maxBytes = useCascadeTag ? 3 : 4; 
      if (bytesToCopy > maxBytes) {
	      bytesToCopy = maxBytes;
      }
      for (count = 0; count < bytesToCopy; count++) {
	      buffer[index++] = uid->uidByte[uidIndex + count];
      }
    }
    if (useCascadeTag) {
      currentLevelKnownBits += 8;
    }
		
    selectDone = 0;
    while (!selectDone) {
      if (currentLevelKnownBits >= 32) {
        buffer[1] = 0x70; 
        buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];
        result = pcd_calculateCRC(buffer, 7, &buffer[7]);
        if (result != STATUS_OK) {
          return result;
        }
        txLastBits		= 0; 
        bufferUsed		= 9;
        responseBuffer	= &buffer[6];
        responseLength	= 3;
      }
      else { 
        txLastBits		= currentLevelKnownBits % 8;
        count			= currentLevelKnownBits / 8;	
        index			= 2 + count;			
        buffer[1]		= (index << 4) + txLastBits;
        bufferUsed		= index + (txLastBits ? 1 : 0);
        responseBuffer	= &buffer[index];
        responseLength	= sizeof(buffer) - index;
      }
			
      rxAlign = txLastBits;										
      pcd_writeRegister(BitFramingReg, (rxAlign << 4) + txLastBits);	
			
      result = pcd_transceiveData(buffer, bufferUsed, responseBuffer, &responseLength, &txLastBits, rxAlign, 0);
      if (result == STATUS_COLLISION) { 
        result = pcd_readRegister(CollReg); 
        if (result & 0x20) { 
          return STATUS_COLLISION; 
        }
        byte collisionPos = result & 0x1F; 
        if (collisionPos == 0) {
          collisionPos = 32;
        }
        if (collisionPos <= currentLevelKnownBits) { 
          return STATUS_INTERNAL_ERROR;
        }
        currentLevelKnownBits = collisionPos;
        count			= (currentLevelKnownBits - 1) % 8; 
        index			= 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0); 
        buffer[index]	|= (1 << count);
      }
      else if (result != STATUS_OK) {
	      return result;
      }
      else { 
        if (currentLevelKnownBits >= 32) {
          selectDone = 1; 
        }
        else { 
          currentLevelKnownBits = 32;
        }
      }
    }
		
    index			= (buffer[2] == PICC_CMD_CT) ? 3 : 2; 
    bytesToCopy		= (buffer[2] == PICC_CMD_CT) ? 3 : 4;
    for (count = 0; count < bytesToCopy; count++) {
      uid->uidByte[uidIndex + count] = buffer[index++];
    }
		
    
    if (responseLength != 3 || txLastBits != 0) { 
      return STATUS_ERROR;
    }
    result = pcd_calculateCRC(responseBuffer, 1, &buffer[2]);
    if (result != STATUS_OK) {
      return result;
    }
    if ((buffer[2] != responseBuffer[1]) || (buffer[3] != responseBuffer[2])) {
      return STATUS_CRC_WRONG;
    }
    if (responseBuffer[0] & 0x04) { 
      cascadeLevel++;
    }
    else {
      uidComplete = 1;
      uid->sak = responseBuffer[0];
    }
  } 
	
  
  uid->size = 3 * cascadeLevel + 1;
	
  return STATUS_OK;
} // End picc_select()

byte picc_requestA(byte *bufferATQA, byte *bufferSize) {
  PDEBUG("Inside function picc_reqA\n");

  return picc_reqA_or_wupA(PICC_CMD_REQA, bufferATQA, bufferSize);

} // End picc_requestA()

byte picc_wakeupA(byte *bufferATQA,	byte *bufferSize) {
  PDEBUG("Inside function picc_wupA\n");

  return picc_reqA_or_wupA(PICC_CMD_WUPA, bufferATQA, bufferSize);

} // End picc_wakeupA

byte picc_reqA_or_wupA(	byte command, byte *bufferATQA,	byte *bufferSize) {
  PDEBUG("Inside function picc_reqA_or_wupA\n");

  byte validBits;
  byte status;
	
  if (bufferATQA == 0 || *bufferSize < 2) {
    return STATUS_NO_ROOM;
  }
  pcd_clearRegisterBitMask(CollReg, 0x80);
  validBits = 7;
  status = pcd_transceiveData(&command, 1, bufferATQA, bufferSize, &validBits, 0, 0);
  if (status != STATUS_OK) {
    return status;
  }
  if (*bufferSize != 2 || validBits != 0) {		// ATQA must be exactly 16 bits.
    return STATUS_ERROR;
  }
  return STATUS_OK;
} // End picc_reqA_or_wupA()

int picc_isNewCardPresent() {
  byte bufferATQA[2];
  byte bufferSize = sizeof(bufferATQA);
  byte result = picc_requestA(bufferATQA, &bufferSize);
  return (result == STATUS_OK || result == STATUS_COLLISION);
} 

int picc_readCardSerial(Uid *uid) {
  byte result = picc_select(uid, 0);
  return (result == STATUS_OK);
}
