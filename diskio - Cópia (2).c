//**************************************************************
// Includes
//**************************************************************
#include "includes.h"

BYTE IOResult;

//  colocado 123

//**************************************************************
// Disco Genérico para funções diversas
//**************************************************************
//disco = 11
BYTE DiskReadGenerico(BYTE *Dest, BYTE Addr, WORD Count)
{	
	DWORD dw;
	WORD i;
	BYTE result = 0;
	
	switch (Addr) {
		//case 0: *Dest = NANDGetManCod(); break;
		//case 1: *Dest = NANDGetDevCod(); break;
		//case 2: *Dest = NANDGetStatus(); break;
		case 3: Dest[0] = MedeCanalAN(0); Dest[1] = MedeCanalAN(0)>>8;  break;
		case 4: Dest[0] = MedeCanalAN(1); Dest[1] = MedeCanalAN(1)>>8;  break;
		case 5: 
			while ((Count--)&&(result==0))
				result = FileReadBYTE(Dest++);
			break;
		//é assim mesmo, sem o break no final
		case 6: FindData.Setor = 0;
		case 7: 
			*Dest++ = FindNext(BLOCO_FILE);			
			i = 0;
			while (Count--) *Dest++ = FindData.Dados[i++];
			FindData.Setor++;
			break;
		case 8: *Dest = 102; break; //versão so software
		case 9:			
			dw = FileSize();
			*Dest++ = dw;			*Dest++ = dw>>8;
			*Dest++ = dw>>16;		*Dest++ = dw>>24;
			*Dest++ = 0x55;
			break;
		case 10:
			EspacoLivreFAT(&dw);
			*Dest++ = dw;			*Dest++ = dw>>8;
			*Dest++ = dw>>16;		*Dest++ = dw>>24;
			*Dest++ = 0x55;			
			break;
		default: break;
	}
	
	return result;
}

DWORD GetDW(BYTE *s)
{	
	DWORD r;
	r  = s[3]; r = r<<8;
	r |= s[2]; r = r<<8;
	r |= s[1]; r = r<<8;
	r |= s[0]; 
	return r;
}

//disco = 11
BYTE DiskWriteGenerico(BYTE *S, BYTE Addr, WORD Count)
{	
	BYTE result = 0;
	
	switch (Addr) {
		case 0: NANDEraseBlock(GetDW(S)); break;
		case 1: LED_TRIGGER = S[0]; break;
		case 2: PulsoResetAltera(); break;
		case 3: while (Count--) ByteProgAlt(*S++); 	break;
		case 4: result = AbreArquivo((char*)S); 	break;
		case 5: result = DeletaArquivo((char*)S); 	break;
		case 6: result = NovoArquivo((char*)S); 	break;
		case 7: result = FechaArquivo(); 			break;
		case 8:
			while ((Count--)&&(result==0)) 
				result = FileWriteBYTE(*S++);
			break;
		case 9: result = FileSeek(GetDW(S)); 	break;
		case 10: SetFileIndex(*S); break;
		case 11: ALT_TRIS(*S); break;
		default : break;
	}
		
	return result;
}

//**************************************************************
// Memória EEPROM
//**************************************************************
#define xPORTE(a)	PORTE = (PORTE&0xFF00)|(a&0x00FF)

BYTE Send25LC640(BYTE d)
{
	char i,r,j;
	
	r = 0;
	for (i=0;i<8;i++)
	{		
		FLASH_SDI = (d&0x80)>0; 
		for (j=0;j<40;j++);
		
		FLASH_SCK = 1; 
		for (j=0;j<40;j++);
		
		r = r<<1;
		d = d<<1;
		r |= FLASH_SDO;
		FLASH_SCK = 0; 
		for (j=0;j<40;j++);					
	}	

	return r;
}

#define EnableEEPROM() FLASH_CE = 0;
#define DisbaleEEPROM()	FLASH_CE = 1;

void WriteEnableEEPROM(void)
{
	EnableEEPROM();	
	Send25LC640(0x01);	
	Send25LC640(0);
	DisbaleEEPROM();

	EnableEEPROM();	
	Send25LC640(0x06);	
	DisbaleEEPROM();
}

BYTE ReadStatusEEPROM(void) //read status register
{
	BYTE status;
	
	EnableEEPROM();	
	Send25LC640(0x05);
	status = Send25LC640(0);	
	DisbaleEEPROM();	
	
	return status;
}

//Texe: 512bytes = 2568us, 200 bytes/ms (06.10.09)
//disco = 12
BYTE DiskReadEEPROM(BYTE *Dest, DWORD Addr, WORD Count)
{	
	EnableEEPROM();

	Send25LC640(0x03);
	Send25LC640((Addr >> 8) & 0xFF);
	Send25LC640((Addr >> 0) & 0xFF);	
	
	while (Count--) *Dest++ = Send25LC640(0);
	
	DisbaleEEPROM();
		
	return 0;
}	

BYTE DiskWriteEEPROMx(BYTE *Source, DWORD Addr, WORD Count)
{	
	WriteEnableEEPROM();
	
	EnableEEPROM();	
	
	Send25LC640(0x02);	
	Send25LC640((Addr >> 8) & 0xFF);
	Send25LC640((Addr >> 0) & 0xFF);	
	
	while (Count--)	Send25LC640(*Source++);
	
	DisbaleEEPROM();		
	
	while (ReadStatusEEPROM()&2);
	while (ReadStatusEEPROM()&1);
	
	return 0;
}

//Texe: 512bytes = 46532us, 11 bytes/ms (06.10.09)
//disco = 12
BYTE DiskWriteEEPROM(BYTE *Source, DWORD Addr, WORD Count)
{		
	//grava as paginas de 32 bytes
	while (Count>32)
	{
		DiskWriteEEPROMx(Source,Addr,32);		
		Source 	+= 32;
		Addr 	+= 32;
		Count	-= 32;		
	}	
	
	//grava o restante
	DiskWriteEEPROMx(Source,Addr,Count);		
		
	return 0;
}

//**************************************************************
// Altera
//**************************************************************
//Texe: 512bytes = 180us, 2.84 Kbytes/ms (06.10.09)
//disco = 14
//disco = 14
BYTE DiskReadAltera(BYTE *Dest, BYTE Addr, WORD Count)
{
	if (!AlteraOK) return 1;

	xPORTE(0b10100011);
	
	SET_BUS_DIR_OUT;
	
	//endereço desejado
	BusData = Addr;
	Nop();	Nop();	Nop();	Nop();
	MEM_ALE = 1;	
	Nop();	Nop();	Nop();	Nop();
	Nop();	Nop();	Nop();	Nop(); Nop();	Nop();
	MEM_ALE = 0;
	Nop();	Nop();	Nop();	Nop();
		
	SET_BUS_DIR_IN;
	
	while (Count--)
	{		
		MEM_RE = 0;
		Nop();	Nop();	Nop();	Nop();	Nop();	Nop();
		Nop();	Nop();	Nop();	Nop(); Nop();	Nop();
		*Dest++ = BusData;		
		MEM_RE = 1;
		Nop();	Nop();	Nop();	Nop();
	}
	
	ALT_CE = 1;
	
	return 0;		
}

//disco = 14
BYTE DiskWriteAltera(BYTE *Source, BYTE Addr, WORD Count)
{		
	if (!AlteraOK) return 1;

	xPORTE(0b10100011);
	
	SET_BUS_DIR_OUT;
	
	//endereço desejado
	BusData = Addr;
	Nop();	Nop();	Nop();	Nop();
	MEM_ALE = 1;
	Nop();	Nop();	Nop();	Nop();
	Nop();	Nop();	Nop();	Nop(); Nop();	Nop();
	MEM_ALE = 0;
	Nop();	Nop();	Nop();	Nop();
	
	while (Count--)
	{		
		BusData = *Source++;
		Nop();	Nop();	Nop();	Nop();
		MEM_WE = 0;
		Nop();	Nop();	Nop();	Nop(); Nop();	Nop();
		Nop();	Nop();	Nop();	Nop(); Nop();	Nop();
		MEM_WE = 1; 
		Nop();	Nop();	Nop();	Nop();
	}
	
	ALT_CE = 1;
	
	return 0;
}
 
BYTE ReadByteAltera(BYTE Addr)
{
	BYTE result;	
	DiskReadAltera(&result,Addr,1);	
	return result;	
}

WORD ReadWordAltera(BYTE Addr)
{	
	WORD r;
	r = ReadByteAltera(Addr);
	r += ReadByteAltera(Addr+1)*256;
	return r;
	

	//DiskReadAltera((BYTE*)(&result),Addr,2);
}


void WriteWordAltera(BYTE Addr, WORD Dado)
{
	WriteByteAltera(Addr,Dado);
	WriteByteAltera(Addr+1,Dado/256);
}


void WriteByteAltera(BYTE Addr, BYTE Dado)
{
	DiskWriteAltera(&Dado,Addr,1);
}

//**************************************************************
// Comunicação com a IHM
//**************************************************************
/*
BYTE DiskReadIHM(BYTE *Dest, BYTE Addr, WORD Count)
{
	return 0;
}

BYTE DiskWriteIHM(BYTE *Source, BYTE Addr, WORD Count)
{		
	return 0;
}
*/

//**************************************************************
// Controle principal dos discos
//**************************************************************
BYTE DiskRead(BYTE Drive, BYTE *Dest, DWORD Addr, WORD Count)
{		
	switch (Drive)	
	{		
		//case 9  : IOResult = DiskReadNANDSpare(Dest,Addr,Count);	break;
		//case 10 : IOResult = DiskReadNAND(Dest,Addr,Count); 		break;
		
		//U8 DiskIOdevNAND(U8 op, U8 *s, U32 a, U32 rcnt, U32 wcnt)
		case  9 : IOResult = DiskIOdevNAND(doREADS,Dest,Addr,Count,0); break;
		case 10 : IOResult = DiskIOdevNAND(doREAD,Dest,Addr,Count,0); break;
		
		case 11 : IOResult = DiskReadGenerico(Dest,Addr,Count); 	break;
		case 12 : IOResult = DiskReadEEPROM(Dest,Addr,Count); 		break;
//		case 13 : IOResult = DiskReadPC(Dest,Addr,Count);			break;
//		case 14 : IOResult = DiskReadAltera(Dest,Addr,Count);		break;		
		//case 15 : IOResult = DiskReadDisplay(Dest,Addr,Count);		break;
//		case 19 : IOResult = DiskReadIHM(Dest,Addr,Count);			break;
		//case 20 : IOResult = DiskReadRTC(Dest,Addr,Count);			break;
//2011		case devPAR: IOResult = DiskReadPAR(Dest,Addr,Count);		break;
//2011		case devCPAR: IOResult = DiskReadCPAR(Dest,Addr,Count);		break;		
		default:
			IOResult = 1;
			break;			
	}			
	
	return IOResult;
} 
 
BYTE DiskWrite(BYTE Drive, BYTE *Source, DWORD Addr, WORD Count)
{	
	switch (Drive)	
	{		
		//case 9  : IOResult = DiskWriteNANDSpare(Source,Addr,Count);	break;
		//case 10 : IOResult = DiskWriteNAND(Source,Addr,Count); 		break;

		//U8 DiskIOdevNAND(U8 op, U8 *s, U32 a, U32 rcnt, U32 wcnt)
		case  9 : IOResult = DiskIOdevNAND(doWRITES,Source,Addr,0,Count); break;
		case 10 : IOResult = DiskIOdevNAND(doWRITE,Source,Addr,0,Count); break;

		case 11 : IOResult = DiskWriteGenerico(Source,Addr,Count);	break;
		case 12 : IOResult = DiskWriteEEPROM(Source,Addr,Count); 	break;
//		case 13 : IOResult = DiskWritePC(Source,Addr,Count); 		break;
		case 14 : IOResult = DiskWriteAltera(Source,Addr,Count); 	break;	//nao tirar usa para display	
		//case 15 : IOResult = DiskWriteDisplay(Source,Addr,Count); 	break;
//		case 19 : IOResult = DiskWriteIHM(Source,Addr,Count); 		break;
		//case 20 : IOResult = DiskWriteRTC(Source,Addr,Count); 		break;
		default:
			IOResult = 1;
			break;			
	}	
	
	return IOResult;
}

