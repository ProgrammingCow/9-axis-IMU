/*
 * lsm303agr.c
 *
 *  Created on: Oct 29, 2022
 *      Author: Moritz
 */
#include "lsm303agr.h"


/* private function prototypes--------------------------------------------------------------*/


static void lsm303agr_readRegister(lsm303agr *handle, uint8_t address, uint8_t register, uint8_t *buf, uint8_t size);
static void lsm303agr_writeRegister(lsm303agr *handle, uint8_t address, uint8_t register, uint8_t buf, uint8_t size);
static void lsm303agr_getMultiplicator(lsm303agr *handle);


/* private function definitions-------------------------------------------------------------*/


/* function:		lsm303agr_readRegister
 * description:		read lsm303agr register in blocking mode with I2C
 ***************************************************************
 * *handle:		pointer to sensor handle
 * address		I2C address of sensor
 * reg			sensor register
 * *rxBuf		pointer	to reception array
 * size			size of data to be received in byte
 ***************************************************************
 * returns:		-
 */
void lsm303agr_readRegister(lsm303agr *handle, uint8_t address, uint8_t reg, uint8_t *rxBuf, uint8_t size)
{
	HAL_I2C_Mem_Read(handle->hi2c,address<<1,reg|(size==1?0x00:0x80),1,rxBuf,size,10);
}

/* function:		lsm303agr_writeRegister
 * description:		write lsm303agr register in blocking mode with I2C
 ***************************************************************
 * *handle:		pointer to sensor handle
 * address		I2C address of sensor
 * reg			sensor register
 * *txBuf		data to be send
 * size			size of data to be received in byte
 ***************************************************************
 * returns:		-
 */
void lsm303agr_writeRegister(lsm303agr *handle, uint8_t address, uint8_t reg, uint8_t txBuf, uint8_t size)
{
	HAL_I2C_Mem_Write(handle->hi2c,address<<1,reg|(size==1?0x00:0x80),1,&txBuf,size,10);
}

/* function:		lsm303agr_getMultiplicator
 * description:		get the Multiplicator for acceleration values with precision and sensor range
 ***************************************************************
 * *handle:		pointer to sensor handle
 ***************************************************************
 * returns:		-
 */
static void lsm303agr_getMultiplicator(lsm303agr *handle)
{
	//high resolution mode
	if(handle->precision_A == 12
			&& handle->maxAbsValue_A == 2)
		handle->multiplicator_A = 0.00098;
	else if(handle->precision_A == 12
			&& handle->maxAbsValue_A == 4)
		handle->multiplicator_A = 0.00195;
	else if(handle->precision_A == 12
			&& handle->maxAbsValue_A == 8)
		handle->multiplicator_A = 0.0039;
	else if(handle->precision_A == 12
			&& handle->maxAbsValue_A == 16)
		handle->multiplicator_A = 0.01172;
	//normal mode
	else if(handle->precision_A == 10
			&& handle->maxAbsValue_A == 2)
		handle->multiplicator_A = 0.0039;
	else if(handle->precision_A == 10
			&& handle->maxAbsValue_A == 4)
		handle->multiplicator_A = 0.00782;
	else if(handle->precision_A == 10
			&& handle->maxAbsValue_A == 8)
		handle->multiplicator_A = 0.01563;
	else if(handle->precision_A == 10
			&& handle->maxAbsValue_A == 16)
		handle->multiplicator_A = 0.00469;
	//low-power mode
	else if(handle->precision_A == 8
			&& handle->maxAbsValue_A == 2)
		handle->multiplicator_A = 0.01563;
	else if(handle->precision_A == 8
			&& handle->maxAbsValue_A == 4)
		handle->multiplicator_A = 0.03126;
	else if(handle->precision_A == 8
			&& handle->maxAbsValue_A == 8)
		handle->multiplicator_A = 0.06252;
	else if(handle->precision_A == 8
			&& handle->maxAbsValue_A == 16)
		handle->multiplicator_A = 0.18758;

	else
		handle->multiplicator_A = 0.0;
}


/*
 * public function definitions--------------------------------------------------------------
 */

/* function:		lsm303agr_config
 * description:		setup the lsm303agr sensor handle and configure the setup registers
 ***************************************************************
 * *handle:		pointer to sensor handle
 * *I2C_hi2c		pointer to SPI handle
 * I2C_hdma_rx		I2C rx to memory DMA handle
 * I2C_hdma_tx		memory to I2C tx DMA handle
 ***************************************************************
 * returns:		-
 */
void lsm303agr_config(lsm303agr *handle, I2C_HandleTypeDef *I2C_hi2c, DMA_HandleTypeDef *I2C_hdma_rx, DMA_HandleTypeDef *I2C_hdma_tx)
{
	//setup sensor related variables
	handle->hi2c = I2C_hi2c;
	handle->hdma_rx = I2C_hdma_rx;
	handle->hdma_tx = I2C_hdma_tx;
	handle->precision_A=12;
	handle->maxAbsValue_A=2;
	lsm303agr_getMultiplicator(handle);
	//write setup registers
	lsm303agr_writeRegister(handle, ACCELEROMETER, CTRL_REG3_A, 0x10,1);
	lsm303agr_writeRegister(handle, ACCELEROMETER, CTRL_REG1_A, 0x77,1);
	lsm303agr_writeRegister(handle, ACCELEROMETER, CTRL_REG4_A, 0x08,1);

}

/* function:		lsm303agr_readSensorData_A
 * description:		read lsm303agr acceleration raw data register in non blocking mode.
 * 			Raw Data will be saved in RxBuf array of handle.
 ***************************************************************
 * *handle:		pointer to sensor handle
 ***************************************************************
 * returns:		-
 */
void lsm303agr_readSensorData_A(lsm303agr *handle)
{
	handle->txBuf[0]=OUT_X_L_A | 0x80;
	HAL_I2C_Mem_Read_DMA(handle->hi2c,ACCELEROMETER<<1,handle->txBuf[0],1,&handle->rxBuf[0],6);
}

/* function:		lsm303agr_calcSensorData_A
 * description:		calculate the acceleration of lsm303agr with the raw values in the rxBuf array in the handle.
 * 			lsm303agr_readSensorData_A have to be called before this function and the DMA have to been finished the SPI communication
 * 			to get correct values.
 ***************************************************************
 * *handle:		pointer to sensor handle
 ***************************************************************
 * returns:		-
 */
void lsm303agr_calcSensorData_A(lsm303agr *handle)
{
	int16_t raw[3];
	for(int i=0;i<3;i++)
	{
		raw[i]=handle->rxBuf[2*i]|handle->rxBuf[2*i+1]<<8;
		raw[i]>>=16 - handle->precision_A;
	}
	handle->x_A=raw[0]*handle->multiplicator_A;
	handle->y_A=raw[1]*handle->multiplicator_A;
	handle->z_A=raw[2]*handle->multiplicator_A;
}
