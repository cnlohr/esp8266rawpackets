#define USER_OVERRIDE
//#define NMI_OVERRIDE

//#define USE_wDevTime //Only gives you to the us.


#ifdef USER_OVERRIDE
.section .text

.align 4



.global packet_tx_time

//_addy_of_packet_tx_matchmask:
//	.long packet_tx_matchmask
_addy_of_packet_tx_time:
	.long packet_tx_time
_lendmark:
	.long debugccount
_gpiomark_out:
	.long 0x60000304 //Pin_set and 308 is pin_Clear
_interrupt_cause_loc:
	.long 0x3ff20c24
_check_against_loc:
	.long debugcontrol

.global PreEmpt_USER_Vector
PreEmpt_USER_Vector:
//	movi a0, 3

#if 1
	addi a1, a1, -16
	s32i a2, a1, 4
	s32i a3, a1, 8
	s32i a4, a1, 12

//    rsr     a2, intenable
//    rsr     a3, interrupt
//    and     a2, a2, a3  //Should also mask off 0x3fff but I'm not worried about it.

	rsr a2, interrupt	//We are only looking at bit 0 and the wifi chip should be enabled.s

	//Make sure it's a wifi interrupt.
	bbci a2, 0, _skip_er  //INUM_WDEV_FIQ = 0

	//Next, make sure it's an actual RX interrupt.
	movi a2, 0x3ff20c20
	l32i a3, a2, 0

#ifdef DEBUG_BITMASK
	movi a2, debugcontrol  //0x0100 looks good.
	l32i a2, a2, 0
	and a3, a2, a3
	beqz a3, _skip_er
#else	
	bbci a3, 8, _skip_er_maybe_trigger_tx
#endif


	//TODO: Look at 0x3ff20c84, it flips between 0x2200 and 0x00
	//TODO TODO: Look at 0x3ff210fc -> May be CCOUNT of incoming packet!!!
	//0x3ff2001c contains a pointer to ram -> which starts with 0xc0068640 ... 0x3ffedcec ... 0x3ffec9f4 ... (0 or 0x80640640) (0 or pointer)
	//0x3ff20014 contains a pointer to ram -> which starts with 0xc0074640 ... 0x3ffedcec ... 0x3ffec9f4 ... (0 or 0x80640640) (0 or pointer)
	//0x3ffec9f4 ->	0x80640640
	//0x3ffedcec -> 0x50154b26 (changes on RSSI), 0x97007107(or0), 0, 0x2c4288, 0xc07fcf5c (or 0xffffffff), 0x9fc25560, 0x40d823db
	//
	movi a2, 0x3ff2001c
	l32i a2, a2, 0 //Get pointer to memory structre
	l32i a2, a2, 4 //Second word in structure
	l32i a3, a2, (12+24) //12 is beginning of packet.  We're loking 24 bytes in.

	movi a4, 0x66826682
	bne  a4, a3, _skip_er

	l32i a3, a2, (12+24+16) //Where the time goes (make sure it's 0)
	bnez a3, _skip_er

#ifdef USE_wDevTime
	movi a3, 0x3ff210fc
	l32i a0, a3, 0
#endif
	s32i a0, a2, (12+24+16) //Write in A0 (Ccount) XXX TODO consider bumming 0x3ff210fc


	l32r a2, _lendmark
	s32i a0, a2, 0
	//Tick on the logic analyzer to test precision of interrupts.
	movi a0, 4

	//Twiddle GPIO
	l32r a2, _gpiomark_out
	s32i a0, a2, 0
	s32i a0, a2, 4
	j _skip_er

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Get precise time of tx packets

_skip_er_maybe_trigger_tx:

	//      vvv  Change this number!!!
	//bbci a3, 7, _skip_er	 //XXX XXX TODO HERE NEED RX interrupt notice.
	//Notes:
	// 0x00040000 = Kind of TX?  Not perfect, tho.  (Bit 18)
	// 0x00080000 = Kind of TX?  Seems worse than 0x00040000... Acks too?

/*
	l32r a2, _addy_of_packet_tx_matchmask
	l32i a2, a2, 0
	and a2, a2, a3
	beqz a2, _skip_er
*/
	bbci a3, 18, _skip_er

	l32r a2, _addy_of_packet_tx_time
	l32i a3, a2, 0
	bnez a3, _skip_er
	s32i a0, a2, 0

_skip_er:

	l32i a2, a1, 4
	l32i a3, a1, 8
	l32i a4, a1, 12

	addi a1, a1, 16
#endif
	_call0 _UserExceptionVector_1

#endif


#ifdef NMI_OVERRIDE
.align 4

.global debugccount
.global PreEmpt_NMI_Vector
_lendmark2b:
	.long debugccount2
PreEmpt_NMI_Vector:
	addi a1, a1, -8
	s32i a2, a1, 4
	l32r a2, _lendmark2b
	s32i a0, a2, 0
	l32i a2, a1, 4
	addi a1, a1, 8
//	rsr a0, execsave3
//	rfi 3
	_call0 _NMILevelVector
#endif


#ifdef USER_OVERRIDE

//This code will be memcpy'd over top of _UserExceptionVector, since I can't figure out how to override it with GCC.
.section .relocuser.text
.global replacement_USER_vect
.align 4
replacement_USER_vect:
	_wsr.excsave1	a0

//	_call0 _UserExceptionVector_1

	rsr a0, ccount
	j PreEmpt_USER_Vector

#endif

#ifdef NMI_OVERRIDE

//This code will be memcpy'd over top of _UserExceptionVector, since I can't figure out how to override it with GCC.
.section .relocvec.text
.global replacement_NMI_vect
.align 4
replacement_NMI_vect:
	//Original code.
	//40100020:	13d300        	wsr.excsave3	a0
	//40100023:	014d05        	call0	401014f4 <_NMILevelVector>
	//wsr a0, execsave3
	.byte 0x00
	.byte 0xd3
	.byte 0x13
//	_call0 _NMILevelVector

#ifndef USE_wDevTime
	rsr a0, ccount
#endif
	j PreEmpt_NMI_Vector
//	ill

#endif


/*

//.section .irom0.text
.section .text

//Originally I thought this part had to be in ASM, it does not.
.global wifi_mcb;
.global aaEnqueueRxq
.align 4
//wifilocator:
//	.long wifi_mcb
aaEnqueueRxq:
	_addi a1, a1, -68
	_s32i.n a0, a1, 0
	_s32i.n a2, a1, 4
	_s32i.n a3, a1, 8
	_s32i.n a4, a1, 12
	_s32i.n a5, a1, 16
	_s32i.n a6, a1, 20
	_s32i.n a7, a1, 24
	_s32i.n a8, a1, 28
	_s32i.n a9, a1, 32
	_s32i.n a10, a1, 36
	_s32i.n a11, a1, 40
	_s32i.n a12, a1, 44
	_s32i.n a13, a1, 48
	_s32i.n a14, a1, 52
	_s32i.n a15, a1, 56


//	l32r a0, wifilocator
//	l32i a0, a0, 0
//	beqz a0, __skip
//		callx0 a0
//	__skip:


	l32i a3, a2, 0
	l32i a4, a2, 4
	l32i a5, a2, 8
	l32i a6, a2, 12
	l32i a2, a2, 16
	call0 rx_func

	_l32i.n a0, a1, 0
	_l32i.n a2, a1, 4
	_l32i.n a3, a1, 8
	_l32i.n a4, a1, 12
	_l32i.n a5, a1, 16
	_l32i.n a6, a1, 20
	_l32i.n a7, a1, 24
	_l32i.n a8, a1, 28
	_l32i.n a9, a1, 32
	_l32i.n a10, a1, 36
	_l32i.n a11, a1, 40
	_l32i.n a12, a1, 44
	_l32i.n a13, a1, 48
	_l32i.n a14, a1, 52
	_l32i.n a15, a1, 56
	_addi a1, a1, 68

	j ppEnqueueRxq
*/
