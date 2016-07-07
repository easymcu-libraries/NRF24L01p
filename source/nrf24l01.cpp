/***************************************************************************
The NRF24L01+ library Written by Brennen Ball, © 2007 for PIC MCU
-> ported for AT91SAM7 MCU by SalehJG -> saleh_jamali@ymail.com
-> ported for LPC1768 MCU + Cpp implementation by Morteza Zandi for EasyMCU

  ------> http://www.easymcu.ir
  
  Check out the links above for our tutorials and wiring diagrams

  All text above must be included in any redistribution
***************************************************************************/

#include "nrf24l01.h"


	 NRF24L01::NRF24L01( bool mode, uchar width, bool autoACK, PORT_PIN_ARRAY Pnum, PORT_PIN_ARRAY ce_pin, PORT_PIN_ARRAY irq_pin, PORT_PIN_ARRAY csn_pin, uchar spiNum )	 
{
	if(spiNum == 1)
	{
		spix = &spi1;
	}
	else
	{
		spix = &spi2;		
	}
	spix->init();
	setIrq(irq_pin);
	setCe(ce_pin);
	setCsn(csn_pin);	
	
	init(mode, width,autoACK, Pnum);
	irq_clear_all();
}	 
//====================================================================================
void NRF24L01::set_tx_addr(unsigned char * address, unsigned int len)
{		
	write_register(nrf24l01_TX_ADDR, address, len);
}
//====================================================================================
void NRF24L01::set_rx_addr(unsigned char * address, unsigned int len, unsigned char rxpipenum)
{	
	if(rxpipenum > 5)
		return;
		
	write_register(nrf24l01_RX_ADDR_P0 + rxpipenum, address, len);
}
//====================================================================================

//powers up the 24L01 with all necessary delays
//this function takes the existing contents of the CONFIG register and sets the PWR_UP 
//the argument rx_active_mode is only used if the user is setting up the
//  24L01 as a receiver.  If the argument is false, the receiver will remain in
//  standby mode and not monitor for packets.  If the argument is true, the CE
//  pin will be set and the 24L01 will monitor for packets.  In TX mode, the value
//  of this argument is insignificant.
//note: if the read value of the CONFIG register already has the PWR_UP bit set, this function
//  exits in order to not make an unecessary register write.
void NRF24L01::power_up(bool rx_active_mode)
{
	unsigned char config;
	
	read_register(nrf24l01_CONFIG, &config, 1);
	
	if((config & nrf24l01_CONFIG_PWR_UP) != 0)
		return;
		
	config |= nrf24l01_CONFIG_PWR_UP;
	
	write_register(nrf24l01_CONFIG, &config, 1);
	
	delay.us(1500);
	
	if((config & nrf24l01_CONFIG_PRIM_RX) == 0)
			CEl();
	else
	{
		if(rx_active_mode != false)
				CEh();
		else
				CEl();
	}
}
//====================================================================================

//powers up the 24L01 with all necessary delays
//this function allows the user to set the contents of the CONFIG register, but the function
//  sets the PWR_UP bit in the CONFIG register, so the user does not need to.
//the argument rx_active_mode is only used if the user is setting up the
//  24L01 as a receiver.  If the argument is false, the receiver will remain in
//  standby mode and not monitor for packets.  If the argument is true, the CE
//  pin will be set and the 24L01 will monitor for packets.  In TX mode, the value
//  of this argument is insignificant.
void NRF24L01::power_up_param(bool rx_active_mode, unsigned char config)
{
	unsigned char test;
	
	config |= nrf24l01_CONFIG_PWR_UP;
	
	write_register(nrf24l01_CONFIG, &config, 1);
	csnLow();
	test = spix->read(nrf24l01_CONFIG);		//SPI_NRF::read_register(nrf24l01_CONFIG);	
	csnHigh();
	test = test;
//	delay.ms(800);

	if((config & nrf24l01_CONFIG_PRIM_RX) == 0)
			CEl();
	else
	{
		if(rx_active_mode != false)
				CEh();
		else
				CEl();
	}
}
//====================================================================================

//powers down the 24L01
//this function takes the existing contents of the CONFIG register and simply
//  clears the PWR_UP bit in the CONFIG register.
//note: if the read value of the CONFIG register already has the PWR_UP bit cleared, this 
//  function exits in order to not make an unecessary register write.
void NRF24L01::power_down()
{
	unsigned char config;
	
	read_register(nrf24l01_CONFIG, &config, 1);
	
	if((config & nrf24l01_CONFIG_PWR_UP) == 0)
		return;
	
	config &= (~nrf24l01_CONFIG_PWR_UP);
	
	write_register(nrf24l01_CONFIG, &config, 1);

		CEl();
}
//====================================================================================

//powers down the 24L01
//this function allows the user to set the contents of the CONFIG register, but the function
//  clears the PWR_UP bit in the CONFIG register, so the user does not need to.
void NRF24L01::power_down_param(unsigned char config)
{
	config &= (~nrf24l01_CONFIG_PWR_UP);
	
	write_register(nrf24l01_CONFIG, &config, 1);

	CEl();//CEh();
}
//====================================================================================
unsigned char NRF24L01::write(uchar * data, unsigned int len, bool transmmit)
{
	unsigned char status;
	
	io.reset(transfer_monitor_pin);
	csnLow();
	status = spix->write(nrf24l01_W_TX_PAYLOAD, data, len);
	csnHigh();
	
//	delay.us(1);
	if(transmmit == true)
		transmit();
//	delay.us(1);
	
	transmit_complete();

	io.set(transfer_monitor_pin);
	
	return status;
}
//========================================================================================
//executes the R_RX_PAYLOAD instruction
//unsigned char * data is the actual payload that has been received by the nrf24l01.
//	The user must size data according to the payload width specified to the nrf24l01.
//	This variable is filled by this function, so individual byte values need not be
//	initialized by the user.
//unsigned int len is the length of the payload being clocked out of the nrf24l01 (this
//	should be sized according to the payload length specified to the nrf24l01).
//returns the value of the STATUS register
unsigned char NRF24L01::read(uchar * data, unsigned int len, bool transmit)	//_rx_payload
{
	unsigned char status;
	
	io.reset(transfer_monitor_pin);
	
	CEl();
	csnLow();
	status = spix->read(nrf24l01_R_RX_PAYLOAD, data, len);
	csnHigh();
	if(transmit)
		delay.ms(40);
	CEh();
	
	clear_flush();
	io.set(transfer_monitor_pin);	
	
	return status;
}
//==========================================================================================
//executes the FLUSH_TX SPI operation
//this funciton empties the contents of the TX FIFO
//returns the value of the STATUS register
unsigned char NRF24L01::flush_tx()
{
	uchar res;
	
	csnLow();
	res = spix->write(nrf24l01_FLUSH_TX, null, 0);
	csnHigh();
	
	return res;
}
//===================================================================================================
//executes the FLUSH_RX SPI operation
//this funciton empties the contents of the RX FIFO
//returns the value of the STATUS register
unsigned char NRF24L01::flush_rx()
{
	uchar res;
	
	csnLow();
	res = spix->write(nrf24l01_FLUSH_RX, null, 0);
	csnHigh();
	
	return res;
}
//===================================================================================================
//executes the REUSE_TX_PL SPI operation
//this funciton allows the user to constantly send a packet repeatedly when issued.
//returns the value of the STATUS register
unsigned char NRF24L01::reuse_tx_pl()
{
	uchar res;
	
	csnLow();
	res = spix->write(nrf24l01_REUSE_TX_PL, null, 0);
	csnHigh();
	
	return res;
}
//===================================================================================================

//executes the FLUSH_TX SPI operation
//this funciton does nothing
//returns the value of the STATUS register
unsigned char NRF24L01::nop()
{
	uchar res;
//	return spix->write(nrf24l01_NOP, null, 0);
	
	csnLow();
	res = spix->transfer((uchar)nrf24l01_NOP);
	csnHigh();
//	io.set(P22);
	
	return res;
}
//===================================================================================================

//transmits the current tx payload
void NRF24L01::transmit()
{
	CEh();
	delay.ms(40); 
	CEl();
}
//====================================================================================
//====================================================================================
  void NRF24L01::initialize(unsigned char config,
						 unsigned char opt_rx_active_mode,  
						 unsigned char en_aa, 
						 unsigned char en_rxaddr, 
						 unsigned char setup_aw, 
						 unsigned char setup_retr, 
						 unsigned char rf_ch, 
						 unsigned char rf_setup, 
						 unsigned char * rx_addr_p0, 
						 unsigned char * rx_addr_p1, 
						 unsigned char rx_addr_p2, 
						 unsigned char rx_addr_p3, 
						 unsigned char rx_addr_p4, 
						 unsigned char rx_addr_p5, 
						 unsigned char * tx_addr, 
						 unsigned char rx_pw_p0, 
						 unsigned char rx_pw_p1, 
						 unsigned char rx_pw_p2, 
						 unsigned char rx_pw_p3, 
						 unsigned char rx_pw_p4, 
						 unsigned char rx_pw_p5)
{
unsigned char data[5];

	data[0] = en_aa;
	write_register(nrf24l01_EN_AA, data, 1);

	data[0] = en_rxaddr;
	write_register(nrf24l01_EN_RXADDR, data, 1);

	data[0] = setup_aw;
	write_register(nrf24l01_SETUP_AW, data, 1);

	data[0] = setup_retr;
	write_register(nrf24l01_SETUP_RETR, data, 1);

	data[0] = rf_ch;
	write_register(nrf24l01_RF_CH, data, 1);

	data[0] = rf_setup;
	write_register(nrf24l01_RF_SETUP, data, 1);

	if(rx_addr_p0 != null)
		set_rx_addr(rx_addr_p0, 5, 0);
	else
	{
		data[0] = nrf24l01_RX_ADDR_P0_B0_DEFAULT_VAL;
		data[1] = nrf24l01_RX_ADDR_P0_B1_DEFAULT_VAL;
		data[2] = nrf24l01_RX_ADDR_P0_B2_DEFAULT_VAL;
		data[3] = nrf24l01_RX_ADDR_P0_B3_DEFAULT_VAL;
		data[4] = nrf24l01_RX_ADDR_P0_B4_DEFAULT_VAL;
		
		set_rx_addr(data, 5, 0);
	}

	if(rx_addr_p1 != null)
		set_rx_addr(rx_addr_p1, 5, 1);
	else
	{
		data[0] = nrf24l01_RX_ADDR_P1_B0_DEFAULT_VAL;
		data[1] = nrf24l01_RX_ADDR_P1_B1_DEFAULT_VAL;
		data[2] = nrf24l01_RX_ADDR_P1_B2_DEFAULT_VAL;
		data[3] = nrf24l01_RX_ADDR_P1_B3_DEFAULT_VAL;
		data[4] = nrf24l01_RX_ADDR_P1_B4_DEFAULT_VAL;
		
		set_rx_addr(data, 5, 1);
	}

	data[0] = rx_addr_p2;
	set_rx_addr(data, 1, 2);

	data[0] = rx_addr_p3;
	set_rx_addr(data, 1, 3);

	data[0] = rx_addr_p4;
	set_rx_addr(data, 1, 4);

	data[0] = rx_addr_p5;
	set_rx_addr(data, 1, 5);

	if(tx_addr != null)
		set_tx_addr(tx_addr, 5);
	else
	{
		data[0] = nrf24l01_TX_ADDR_B0_DEFAULT_VAL;
		data[1] = nrf24l01_TX_ADDR_B1_DEFAULT_VAL;
		data[2] = nrf24l01_TX_ADDR_B2_DEFAULT_VAL;
		data[3] = nrf24l01_TX_ADDR_B3_DEFAULT_VAL;
		data[4] = nrf24l01_TX_ADDR_B4_DEFAULT_VAL;
		
		set_tx_addr(data, 5);
	}

	data[0] = rx_pw_p0;
	write_register(nrf24l01_RX_PW_P0, data, 1);

	data[0] = rx_pw_p1;
	write_register(nrf24l01_RX_PW_P1, data, 1);

	data[0] = rx_pw_p2;
	write_register(nrf24l01_RX_PW_P2, data, 1);

	data[0] = rx_pw_p3;
	write_register(nrf24l01_RX_PW_P3, data, 1);

	data[0] = rx_pw_p4;
	write_register(nrf24l01_RX_PW_P4, data, 1);

	data[0] = rx_pw_p5;
	write_register(nrf24l01_RX_PW_P5, data, 1);

	if((config & nrf24l01_CONFIG_PWR_UP) != 0)
		power_up_param(opt_rx_active_mode, config);
	else
		power_down_param(config);
}

//initializes the 24L01 to all default values except the PWR_UP and PRIM_RX bits
//this function also disables the auto-ack feature on the chip (EN_AA register is 0)
//bool rx is true if the device should be a receiver and false if it should be
//  a transmitter.
//unsigned char payload_width is the payload width for pipe 0.  All other pipes
//  are left in their default (disabled) state.
//bool enable_auto_ack controls the auto ack feature on pipe 0.  If true, auto-ack will
//  be enabled.  If false, auto-ack is disabled.
void NRF24L01::init(bool rx, unsigned char p0_payload_width, bool enable_auto_ack, PORT_PIN_ARRAY Pnum)	//ialize_debug
{
	unsigned char config;
	unsigned char en_aa;
	
	config = nrf24l01_CONFIG_DEFAULT_VAL | nrf24l01_CONFIG_PWR_UP;
	transfer_monitor_pin = Pnum;
	
	if(enable_auto_ack != false)
		en_aa = nrf24l01_EN_AA_ENAA_P0;
	else
		en_aa = nrf24l01_EN_AA_ENAA_NONE;
	
	if(rx == true)
		config = config | nrf24l01_CONFIG_PRIM_RX;
		
	initialize(config, 
						true,
						en_aa, 
						nrf24l01_EN_RXADDR_DEFAULT_VAL, 
						nrf24l01_SETUP_AW_DEFAULT_VAL, 
						nrf24l01_SETUP_RETR_DEFAULT_VAL, 
						nrf24l01_RF_CH_DEFAULT_VAL, 
						nrf24l01_RF_SETUP_DEFAULT_VAL,  
						null, 
						null, 
						nrf24l01_RX_ADDR_P2_DEFAULT_VAL, 
						nrf24l01_RX_ADDR_P3_DEFAULT_VAL, 
						nrf24l01_RX_ADDR_P4_DEFAULT_VAL, 
						nrf24l01_RX_ADDR_P5_DEFAULT_VAL, 
						null, 
						p0_payload_width, 
						nrf24l01_RX_PW_P1_DEFAULT_VAL, 
						nrf24l01_RX_PW_P2_DEFAULT_VAL, 
						nrf24l01_RX_PW_P3_DEFAULT_VAL, 
						nrf24l01_RX_PW_P4_DEFAULT_VAL, 
						nrf24l01_RX_PW_P5_DEFAULT_VAL);
}
//initializes only the CONFIG register and pipe 0's payload width
//the primary purpose of this function is to allow users with microcontrollers with
//  extremely small program memories to still be able to init their 24L01.  This code
//  should have a smaller footprint than the above init functions.
//when using this method, the 24L01 MUST have its default configuration loaded
//  in all registers to work.  It is recommended that the device be reset or
//  have its power cycled immediately before this code is run.
//in normal circumstances, the user should use nrf24l01_initialize() rather than this
//  function, since this function does not set all of the register values.
void NRF24L01::initialize_debug_lite(bool rx, unsigned char p0_payload_width)
{
	unsigned char config;
	
	config = nrf24l01_CONFIG_DEFAULT_VAL;
	
	if(rx != false)
		config |= nrf24l01_CONFIG_PRIM_RX;
		
	write_register(nrf24l01_RX_PW_P0, &p0_payload_width, 1);
	power_up_param(true, config);
}
//clear all interrupts in the status register
void NRF24L01::irq_clear_all()
{
	unsigned char data = nrf24l01_STATUS_RX_DR | nrf24l01_STATUS_TX_DS |nrf24l01_STATUS_MAX_RT;
	CEl();
	write_register(nrf24l01_STATUS, &data, 1);
//	delay.us(200);
	CEh();
}
bool NRF24L01::irq_pin_active()
{
	return(!(io.read(irqP)));
}
unsigned char NRF24L01::get_status()
{
	return nop();
}




//returns true if RX_DR interrupt is active, false otherwise
bool NRF24L01::irq_rx_dr_active()
{
	return (get_status() & nrf24l01_STATUS_RX_DR);
}

//returns true if TX_DS interrupt is active, false otherwise
bool NRF24L01::irq_tx_ds_active()
{
	return (get_status() & nrf24l01_STATUS_TX_DS);
}

//returns true if MAX_RT interrupt is active, false otherwise
bool NRF24L01::irq_max_rt_active()
{
	return (get_status() & nrf24l01_STATUS_MAX_RT);
}


//clears only the RX_DR interrupt
void NRF24L01::irq_clear_rx_dr()
{
	unsigned char data = nrf24l01_STATUS_RX_DR;
	
	write_register(nrf24l01_STATUS, &data, 1); 
}

//clears only the TX_DS interrupt
void NRF24L01::irq_clear_tx_ds()
{
	unsigned char data = nrf24l01_STATUS_TX_DS;
	
	write_register(nrf24l01_STATUS, &data, 1); 
}

//clears only the MAX_RT interrupt
void NRF24L01::irq_clear_max_rt()
{
	unsigned char data = nrf24l01_STATUS_MAX_RT;
	
	write_register(nrf24l01_STATUS, &data, 1); 
}

//returns the current pipe in the 24L01's STATUS register
unsigned char NRF24L01::get_rx_pipe()
{
	return get_rx_pipe_from_status(get_status());
}

unsigned char NRF24L01::get_rx_pipe_from_status(unsigned char status)
{
	return ((status & 0xE) >> 1);
}

//flush both fifos and clear interrupts
void NRF24L01::clear_flush()
{
	flush_rx();
	flush_tx();
	irq_clear_all();
}
//===============================
unsigned char NRF24L01::read_register(unsigned char regnumber, unsigned char * data, unsigned int len)
{
	uchar res;
	
	csnLow();
	res = spix->read(regnumber | nrf24l01_R_REGISTER_DATA, data, len);
	csnHigh();
	
	return res;
}
//==============================================
void NRF24L01::get_all_registers(unsigned char * data)	
{
	unsigned int outer;
	unsigned int inner;
	unsigned int dataloc = 0;   
	unsigned char buffer[5];

	for(outer = 0; outer <= 0x17; outer++)
	{
		spix->read(outer, buffer, 5);	
		delay.ms(1);
		delay.ms(1);
		for(inner = 0; inner < 5; inner++)
		{
			if(inner >= 1 && (outer != 0x0A && outer != 0x0B && outer != 0x10))
				break;
	
			data[dataloc] = buffer[inner];
			delay.ms(1);
			dataloc++;
		}
	}
}

//-------------------------------------------------------------------------------
unsigned char NRF24L01::write_register(unsigned char address,unsigned char *value,unsigned char len)
{
	uchar res;
	
	csnLow();
	res = spix->write(nrf24l01_W_REGISTER | (address & nrf24l01_W_REGISTER_DATA),value,len);
	csnHigh();
	
	return res;
}

//======================================
__inline void NRF24L01::CEh()
{
	io.set(ceP);		
}
//================
__inline void NRF24L01::CEl()
{
	 io.reset(ceP);			
}
//==================
//unsigned char NRF24L01::get_input(unsigned int pin)
//{
//return(( LPC_GPIO0->FIOPIN & pin ));	//>>pin

//}	 
//------------------------------------------
void NRF24L01::transmit_complete(void)
{
		uint8_t status1, status2, status3;
	
		do
		{												  
			status1= irq_pin_active()   ;
			status2= irq_tx_ds_active() ;
			status3= irq_max_rt_active();
		}
		while(!(status1 &&(status2 || status3)));		
		
		irq_clear_all(); //clear all interrupts in the 24L01
}

//--------------------------------------------

unsigned char NRF24L01::available(void)
{
			uint8_t status1, status2;
			do						     
			{
			  status1 = irq_pin_active();
				status2 = irq_rx_dr_active();
			}while(!(status1 && status2));
			return 1;	
}

void NRF24L01::setIrq(PORT_PIN_ARRAY irq_pin)
{
	irqP = irq_pin;
io.mode(irqP, INPUT_PULLUP);
}

void NRF24L01::setCe(PORT_PIN_ARRAY ce_pin)
{
	ceP = ce_pin;
	io.mode(ceP, OUTPUT);
}

void NRF24L01::setCsn(PORT_PIN_ARRAY csn_pin)
{
	csnP = csn_pin;
	io.mode(csnP, OUTPUT);
	io.set(csnP);
}

__inline void NRF24L01::csnHigh(void)
{
	io.set(csnP);
}

__inline void NRF24L01::csnLow(void)
{
	io.reset(csnP);
}

