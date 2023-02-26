/** Z80: portable Z80 emulator *******************************/
/**                                                         **/
/**                           Z80.c                         **/
/**                                                         **/
/** This file contains implementation for Z80 CPU. Don't    **/
/** forget to provide RdZ80(), WrZ80(), InZ80(), OutZ80(),  **/
/** LoopZ80(), and PatchZ80() functions to accomodate the   **/
/** emulated machine's architecture.                        **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994,1995,1996,1997       **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/   
/**     changes to this file.                               **/
/*************************************************************/

#include "Z80.h"
#include "Tables.h"
#include <stdio.h>

/** INLINE ***************************************************/
/** Different compilers inline C functions differently.     **/
/*************************************************************/
#define INLINE inline

/** System-Dependent Stuff ***********************************/
/** This is system-dependent code put here to speed things  **/
/** up. It has to stay inlined to be fast.                  **/
/*************************************************************/
byte *RAM;


#define S(Fl)        R->AF.B.l|=Fl
#define R(Fl)        R->AF.B.l&=~(Fl)
#define FLAGS(Rg,Fl) R->AF.B.l=Fl|ZSTable[Rg]

#define M_RLC(Rg)      \
  R->AF.B.l=Rg>>7;Rg=(Rg<<1)|R->AF.B.l;R->AF.B.l|=PZSTable[Rg]
#define M_RRC(Rg)      \
  R->AF.B.l=Rg&0x01;Rg=(Rg>>1)|(R->AF.B.l<<7);R->AF.B.l|=PZSTable[Rg]
#define M_RL(Rg)       \
  if(Rg&0x80)          \
  {                    \
    Rg=(Rg<<1)|(R->AF.B.l&C_FLAG); \
    R->AF.B.l=PZSTable[Rg]|C_FLAG; \
  }                    \
  else                 \
  {                    \
    Rg=(Rg<<1)|(R->AF.B.l&C_FLAG); \
    R->AF.B.l=PZSTable[Rg];        \
  }
#define M_RR(Rg)       \
  if(Rg&0x01)          \
  {                    \
    Rg=(Rg>>1)|(R->AF.B.l<<7);     \
    R->AF.B.l=PZSTable[Rg]|C_FLAG; \
  }                    \
  else                 \
  {                    \
    Rg=(Rg>>1)|(R->AF.B.l<<7);     \
    R->AF.B.l=PZSTable[Rg];        \
  }
  
#define M_SLA(Rg)      \
  R->AF.B.l=Rg>>7;Rg<<=1;R->AF.B.l|=PZSTable[Rg]
#define M_SRA(Rg)      \
  R->AF.B.l=Rg&C_FLAG;Rg=(Rg>>1)|(Rg&0x80);R->AF.B.l|=PZSTable[Rg]

#define M_SLL(Rg)      \
  R->AF.B.l=Rg>>7;Rg=(Rg<<1)|0x01;R->AF.B.l|=PZSTable[Rg]
#define M_SRL(Rg)      \
  R->AF.B.l=Rg&0x01;Rg>>=1;R->AF.B.l|=PZSTable[Rg]

#define M_BIT(Bit,Rg)  \
  R->AF.B.l=(R->AF.B.l&~(N_FLAG|Z_FLAG))|H_FLAG|(Rg&(1<<Bit)? 0:Z_FLAG)

#define M_SET(Bit,Rg) Rg|=1<<Bit
#define M_RES(Bit,Rg) Rg&=~(1<<Bit)

#define M_POP(Rg)      \
  R->Rg.B.l=RdZ80(R->SP.W++);R->Rg.B.h=RdZ80(R->SP.W++)
#define M_PUSH(Rg)     \
  WrZ80(--R->SP.W,R->Rg.B.h);WrZ80(--R->SP.W,R->Rg.B.l)

#define M_CALL         \
  J.B.l=RdZ80(R->PC.W++);J.B.h=RdZ80(R->PC.W++);         \
  WrZ80(--R->SP.W,R->PC.B.h);WrZ80(--R->SP.W,R->PC.B.l); \
  R->PC.W=J.W

#define M_JP  J.B.l=RdZ80(R->PC.W++);J.B.h=RdZ80(R->PC.W);R->PC.W=J.W
#define M_JR  R->PC.W+=(offset)RdZ80(R->PC.W)+1
#define M_RET R->PC.B.l=RdZ80(R->SP.W++);R->PC.B.h=RdZ80(R->SP.W++)

#define M_RST(Ad)      \
  WrZ80(--R->SP.W,R->PC.B.h);WrZ80(--R->SP.W,R->PC.B.l);R->PC.W=Ad

#define M_LDWORD(Rg)   \
  R->Rg.B.l=RdZ80(R->PC.W++);R->Rg.B.h=RdZ80(R->PC.W++)

#define M_ADD(Rg)      \
  J.W=R->AF.B.h+Rg;     \
  R->AF.B.l=            \
    (~(R->AF.B.h^Rg)&(Rg^J.B.l)&0x80? V_FLAG:0)| \
    J.B.h|ZSTable[J.B.l]|                        \
    ((R->AF.B.h^Rg^J.B.l)&H_FLAG);               \
  R->AF.B.h=J.B.l       

#define M_SUB(Rg)      \
  J.W=R->AF.B.h-Rg;    \
  R->AF.B.l=           \
    ((R->AF.B.h^Rg)&(R->AF.B.h^J.B.l)&0x80? V_FLAG:0)| \
    N_FLAG|-J.B.h|ZSTable[J.B.l]|                      \
    ((R->AF.B.h^Rg^J.B.l)&H_FLAG);                     \
  R->AF.B.h=J.B.l

#define M_ADC(Rg)      \
  J.W=R->AF.B.h+Rg+(R->AF.B.l&C_FLAG); \
  R->AF.B.l=                           \
    (~(R->AF.B.h^Rg)&(Rg^J.B.l)&0x80? V_FLAG:0)| \
    J.B.h|ZSTable[J.B.l]|              \
    ((R->AF.B.h^Rg^J.B.l)&H_FLAG);     \
  R->AF.B.h=J.B.l

#define M_SBC(Rg)      \
  J.W=R->AF.B.h-Rg-(R->AF.B.l&C_FLAG); \
  R->AF.B.l=                           \
    ((R->AF.B.h^Rg)&(R->AF.B.h^J.B.l)&0x80? V_FLAG:0)| \
    N_FLAG|-J.B.h|ZSTable[J.B.l]|      \
    ((R->AF.B.h^Rg^J.B.l)&H_FLAG);     \
  R->AF.B.h=J.B.l

#define M_CP(Rg)       \
  J.W=R->AF.B.h-Rg;    \
  R->AF.B.l=           \
    ((R->AF.B.h^Rg)&(R->AF.B.h^J.B.l)&0x80? V_FLAG:0)| \
    N_FLAG|-J.B.h|ZSTable[J.B.l]|                      \
    ((R->AF.B.h^Rg^J.B.l)&H_FLAG)

#define M_AND(Rg) R->AF.B.h&=Rg;R->AF.B.l=H_FLAG|PZSTable[R->AF.B.h]
#define M_OR(Rg)  R->AF.B.h|=Rg;R->AF.B.l=PZSTable[R->AF.B.h]
#define M_XOR(Rg) R->AF.B.h^=Rg;R->AF.B.l=PZSTable[R->AF.B.h]
#define M_IN(Rg)  Rg=InZ80(R->BC.B.l);R->AF.B.l=PZSTable[Rg]|(R->AF.B.l&C_FLAG)

#define M_INC(Rg)       \
  Rg++;                 \
  R->AF.B.l=            \
    (R->AF.B.l&C_FLAG)|ZSTable[Rg]|           \
    (Rg==0x80? V_FLAG:0)|(Rg&0x0F? 0:H_FLAG)

#define M_DEC(Rg)       \
  Rg--;                 \
  R->AF.B.l=            \
    N_FLAG|(R->AF.B.l&C_FLAG)|ZSTable[Rg]| \
    (Rg==0x7F? V_FLAG:0)|((Rg&0x0F)==0x0F? H_FLAG:0)

#define M_ADDW(Rg1,Rg2) \
  J.W=(R->Rg1.W+R->Rg2.W)&0xFFFF;                        \
  R->AF.B.l=                                             \
    (R->AF.B.l&~(H_FLAG|N_FLAG|C_FLAG))|                 \
    ((R->Rg1.W^R->Rg2.W^J.W)&0x1000? H_FLAG:0)|          \
    (((long)R->Rg1.W+(long)R->Rg2.W)&0x10000? C_FLAG:0); \
  R->Rg1.W=J.W

#define M_ADCW(Rg)      \
  I=R->AF.B.l&C_FLAG;J.W=(R->HL.W+R->Rg.W+I)&0xFFFF;           \
  R->AF.B.l=                                                   \
    (((long)R->HL.W+(long)R->Rg.W+(long)I)&0x10000? C_FLAG:0)| \
    (~(R->HL.W^R->Rg.W)&(R->Rg.W^J.W)&0x8000? V_FLAG:0)|       \
    ((R->HL.W^R->Rg.W^J.W)&0x1000? H_FLAG:0)|                  \
    (J.W? 0:Z_FLAG)|(J.B.h&S_FLAG);                            \
  R->HL.W=J.W
   
#define M_SBCW(Rg)      \
  I=R->AF.B.l&C_FLAG;J.W=(R->HL.W-R->Rg.W-I)&0xFFFF;           \
  R->AF.B.l=                                                   \
    N_FLAG|                                                    \
    (((long)R->HL.W-(long)R->Rg.W-(long)I)&0x10000? C_FLAG:0)| \
    ((R->HL.W^R->Rg.W)&(R->HL.W^J.W)&0x8000? V_FLAG:0)|        \
    ((R->HL.W^R->Rg.W^J.W)&0x1000? H_FLAG:0)|                  \
    (J.W? 0:Z_FLAG)|(J.B.h&S_FLAG);                            \
  R->HL.W=J.W

enum Codes
{
  NOP,LD_BC_WORD,LD_xBC_A,INC_BC,INC_B,DEC_B,LD_B_BYTE,RLCA,
  EX_AF_AF,ADD_HL_BC,LD_A_xBC,DEC_BC,INC_C,DEC_C,LD_C_BYTE,RRCA,
  DJNZ,LD_DE_WORD,LD_xDE_A,INC_DE,INC_D,DEC_D,LD_D_BYTE,RLA,
  JR,ADD_HL_DE,LD_A_xDE,DEC_DE,INC_E,DEC_E,LD_E_BYTE,RRA,
  JR_NZ,LD_HL_WORD,LD_xWORD_HL,INC_HL,INC_H,DEC_H,LD_H_BYTE,DAA,
  JR_Z,ADD_HL_HL,LD_HL_xWORD,DEC_HL,INC_L,DEC_L,LD_L_BYTE,CPL,
  JR_NC,LD_SP_WORD,LD_xWORD_A,INC_SP,INC_xHL,DEC_xHL,LD_xHL_BYTE,SCF,
  JR_C,ADD_HL_SP,LD_A_xWORD,DEC_SP,INC_A,DEC_A,LD_A_BYTE,CCF,
  LD_B_B,LD_B_C,LD_B_D,LD_B_E,LD_B_H,LD_B_L,LD_B_xHL,LD_B_A,
  LD_C_B,LD_C_C,LD_C_D,LD_C_E,LD_C_H,LD_C_L,LD_C_xHL,LD_C_A,
  LD_D_B,LD_D_C,LD_D_D,LD_D_E,LD_D_H,LD_D_L,LD_D_xHL,LD_D_A,
  LD_E_B,LD_E_C,LD_E_D,LD_E_E,LD_E_H,LD_E_L,LD_E_xHL,LD_E_A,
  LD_H_B,LD_H_C,LD_H_D,LD_H_E,LD_H_H,LD_H_L,LD_H_xHL,LD_H_A,
  LD_L_B,LD_L_C,LD_L_D,LD_L_E,LD_L_H,LD_L_L,LD_L_xHL,LD_L_A,
  LD_xHL_B,LD_xHL_C,LD_xHL_D,LD_xHL_E,LD_xHL_H,LD_xHL_L,HALT,LD_xHL_A,
  LD_A_B,LD_A_C,LD_A_D,LD_A_E,LD_A_H,LD_A_L,LD_A_xHL,LD_A_A,
  ADD_B,ADD_C,ADD_D,ADD_E,ADD_H,ADD_L,ADD_xHL,ADD_A,
  ADC_B,ADC_C,ADC_D,ADC_E,ADC_H,ADC_L,ADC_xHL,ADC_A,
  SUB_B,SUB_C,SUB_D,SUB_E,SUB_H,SUB_L,SUB_xHL,SUB_A,
  SBC_B,SBC_C,SBC_D,SBC_E,SBC_H,SBC_L,SBC_xHL,SBC_A,
  AND_B,AND_C,AND_D,AND_E,AND_H,AND_L,AND_xHL,AND_A,
  XOR_B,XOR_C,XOR_D,XOR_E,XOR_H,XOR_L,XOR_xHL,XOR_A,
  OR_B,OR_C,OR_D,OR_E,OR_H,OR_L,OR_xHL,OR_A,
  CP_B,CP_C,CP_D,CP_E,CP_H,CP_L,CP_xHL,CP_A,
  RET_NZ,POP_BC,JP_NZ,JP,CALL_NZ,PUSH_BC,ADD_BYTE,RST00,
  RET_Z,RET,JP_Z,PFX_CB,CALL_Z,CALL,ADC_BYTE,RST08,
  RET_NC,POP_DE,JP_NC,OUTA,CALL_NC,PUSH_DE,SUB_BYTE,RST10,
  RET_C,EXX,JP_C,INA,CALL_C,PFX_DD,SBC_BYTE,RST18,
  RET_PO,POP_HL,JP_PO,EX_HL_xSP,CALL_PO,PUSH_HL,AND_BYTE,RST20,
  RET_PE,LD_PC_HL,JP_PE,EX_DE_HL,CALL_PE,PFX_ED,XOR_BYTE,RST28,
  RET_P,POP_AF,JP_P,DI,CALL_P,PUSH_AF,OR_BYTE,RST30,
  RET_M,LD_SP_HL,JP_M,EI,CALL_M,PFX_FD,CP_BYTE,RST38
};

enum CodesCB
{
  RLC_B,RLC_C,RLC_D,RLC_E,RLC_H,RLC_L,RLC_xHL,RLC_A,
  RRC_B,RRC_C,RRC_D,RRC_E,RRC_H,RRC_L,RRC_xHL,RRC_A,
  RL_B,RL_C,RL_D,RL_E,RL_H,RL_L,RL_xHL,RL_A,
  RR_B,RR_C,RR_D,RR_E,RR_H,RR_L,RR_xHL,RR_A,
  SLA_B,SLA_C,SLA_D,SLA_E,SLA_H,SLA_L,SLA_xHL,SLA_A,
  SRA_B,SRA_C,SRA_D,SRA_E,SRA_H,SRA_L,SRA_xHL,SRA_A,
  SLL_B,SLL_C,SLL_D,SLL_E,SLL_H,SLL_L,SLL_xHL,SLL_A,
  SRL_B,SRL_C,SRL_D,SRL_E,SRL_H,SRL_L,SRL_xHL,SRL_A,
  BIT0_B,BIT0_C,BIT0_D,BIT0_E,BIT0_H,BIT0_L,BIT0_xHL,BIT0_A,
  BIT1_B,BIT1_C,BIT1_D,BIT1_E,BIT1_H,BIT1_L,BIT1_xHL,BIT1_A,
  BIT2_B,BIT2_C,BIT2_D,BIT2_E,BIT2_H,BIT2_L,BIT2_xHL,BIT2_A,
  BIT3_B,BIT3_C,BIT3_D,BIT3_E,BIT3_H,BIT3_L,BIT3_xHL,BIT3_A,
  BIT4_B,BIT4_C,BIT4_D,BIT4_E,BIT4_H,BIT4_L,BIT4_xHL,BIT4_A,
  BIT5_B,BIT5_C,BIT5_D,BIT5_E,BIT5_H,BIT5_L,BIT5_xHL,BIT5_A,
  BIT6_B,BIT6_C,BIT6_D,BIT6_E,BIT6_H,BIT6_L,BIT6_xHL,BIT6_A,
  BIT7_B,BIT7_C,BIT7_D,BIT7_E,BIT7_H,BIT7_L,BIT7_xHL,BIT7_A,
  RES0_B,RES0_C,RES0_D,RES0_E,RES0_H,RES0_L,RES0_xHL,RES0_A,
  RES1_B,RES1_C,RES1_D,RES1_E,RES1_H,RES1_L,RES1_xHL,RES1_A,
  RES2_B,RES2_C,RES2_D,RES2_E,RES2_H,RES2_L,RES2_xHL,RES2_A,
  RES3_B,RES3_C,RES3_D,RES3_E,RES3_H,RES3_L,RES3_xHL,RES3_A,
  RES4_B,RES4_C,RES4_D,RES4_E,RES4_H,RES4_L,RES4_xHL,RES4_A,
  RES5_B,RES5_C,RES5_D,RES5_E,RES5_H,RES5_L,RES5_xHL,RES5_A,
  RES6_B,RES6_C,RES6_D,RES6_E,RES6_H,RES6_L,RES6_xHL,RES6_A,
  RES7_B,RES7_C,RES7_D,RES7_E,RES7_H,RES7_L,RES7_xHL,RES7_A,  
  SET0_B,SET0_C,SET0_D,SET0_E,SET0_H,SET0_L,SET0_xHL,SET0_A,
  SET1_B,SET1_C,SET1_D,SET1_E,SET1_H,SET1_L,SET1_xHL,SET1_A,
  SET2_B,SET2_C,SET2_D,SET2_E,SET2_H,SET2_L,SET2_xHL,SET2_A,
  SET3_B,SET3_C,SET3_D,SET3_E,SET3_H,SET3_L,SET3_xHL,SET3_A,
  SET4_B,SET4_C,SET4_D,SET4_E,SET4_H,SET4_L,SET4_xHL,SET4_A,
  SET5_B,SET5_C,SET5_D,SET5_E,SET5_H,SET5_L,SET5_xHL,SET5_A,
  SET6_B,SET6_C,SET6_D,SET6_E,SET6_H,SET6_L,SET6_xHL,SET6_A,
  SET7_B,SET7_C,SET7_D,SET7_E,SET7_H,SET7_L,SET7_xHL,SET7_A
};
  
enum CodesED
{
  DB_00,DB_01,DB_02,DB_03,DB_04,DB_05,DB_06,DB_07,
  DB_08,DB_09,DB_0A,DB_0B,DB_0C,DB_0D,DB_0E,DB_0F,
  DB_10,DB_11,DB_12,DB_13,DB_14,DB_15,DB_16,DB_17,
  DB_18,DB_19,DB_1A,DB_1B,DB_1C,DB_1D,DB_1E,DB_1F,
  DB_20,DB_21,DB_22,DB_23,DB_24,DB_25,DB_26,DB_27,
  DB_28,DB_29,DB_2A,DB_2B,DB_2C,DB_2D,DB_2E,DB_2F,
  DB_30,DB_31,DB_32,DB_33,DB_34,DB_35,DB_36,DB_37,
  DB_38,DB_39,DB_3A,DB_3B,DB_3C,DB_3D,DB_3E,DB_3F,
  IN_B_xC,OUT_xC_B,SBC_HL_BC,LD_xWORDe_BC,NEG,RETN,IM_0,LD_I_A,
  IN_C_xC,OUT_xC_C,ADC_HL_BC,LD_BC_xWORDe,DB_4C,RETI,DB_,LD_R_A,
  IN_D_xC,OUT_xC_D,SBC_HL_DE,LD_xWORDe_DE,DB_54,DB_55,IM_1,LD_A_I,
  IN_E_xC,OUT_xC_E,ADC_HL_DE,LD_DE_xWORDe,DB_5C,DB_5D,IM_2,LD_A_R,
  IN_H_xC,OUT_xC_H,SBC_HL_HL,LD_xWORDe_HL,DB_64,DB_65,DB_66,RRD,
  IN_L_xC,OUT_xC_L,ADC_HL_HL,LD_HL_xWORDe,DB_6C,DB_6D,DB_6E,RLD,
  IN_F_xC,DB_71,SBC_HL_SP,LD_xWORDe_SP,DB_74,DB_75,DB_76,DB_77,
  IN_A_xC,OUT_xC_A,ADC_HL_SP,LD_SP_xWORDe,DB_7C,DB_7D,DB_7E,DB_7F,
  DB_80,DB_81,DB_82,DB_83,DB_84,DB_85,DB_86,DB_87,
  DB_88,DB_89,DB_8A,DB_8B,DB_8C,DB_8D,DB_8E,DB_8F,
  DB_90,DB_91,DB_92,DB_93,DB_94,DB_95,DB_96,DB_97,
  DB_98,DB_99,DB_9A,DB_9B,DB_9C,DB_9D,DB_9E,DB_9F,
  LDI,CPI,INI,OUTI,DB_A4,DB_A5,DB_A6,DB_A7,
  LDD,CPD,IND,OUTD,DB_AC,DB_AD,DB_AE,DB_AF,
  LDIR,CPIR,INIR,OTIR,DB_B4,DB_B5,DB_B6,DB_B7,
  LDDR,CPDR,INDR,OTDR,DB_BC,DB_BD,DB_BE,DB_BF,
  DB_C0,DB_C1,DB_C2,DB_C3,DB_C4,DB_C5,DB_C6,DB_C7,
  DB_C8,DB_C9,DB_CA,DB_CB,DB_CC,DB_CD,DB_CE,DB_CF,
  DB_D0,DB_D1,DB_D2,DB_D3,DB_D4,DB_D5,DB_D6,DB_D7,
  DB_D8,DB_D9,DB_DA,DB_DB,DB_DC,DB_DD,DB_DE,DB_DF,
  DB_E0,DB_E1,DB_E2,DB_E3,DB_E4,DB_E5,DB_E6,DB_E7,
  DB_E8,DB_E9,DB_EA,DB_EB,DB_EC,DB_ED,DB_EE,DB_EF,
  DB_F0,DB_F1,DB_F2,DB_F3,DB_F4,DB_F5,DB_F6,DB_F7,
  DB_F8,DB_F9,DB_FA,DB_FB,DB_FC,DB_FD,DB_FE,DB_FF
};

static void CodesCB(register Z80 *R)
{
  register byte I;

  I=RdZ80(R->PC.W++);
  R->ICount-=CyclesCB[I];
  switch(I)
  {
#include "CodesCB.h"
    default:
		if(R->TrapBadOps) {}
  }
}

static void CodesDDCB(register Z80 *R)
{
  register pair J;
  register byte I;

#define XX IX    
  J.W=R->XX.W+(offset)RdZ80(R->PC.W++);
  I=RdZ80(R->PC.W++);
  R->ICount-=CyclesXXCB[I];
  switch(I)
  {
#include "CodesXCB.h"
    default:
		if(R->TrapBadOps) {}
  }
#undef XX
}

static void CodesFDCB(register Z80 *R)
{
  register pair J;
  register byte I;

#define XX IY
  J.W=R->XX.W+(offset)RdZ80(R->PC.W++);
  I=RdZ80(R->PC.W++);
  R->ICount-=CyclesXXCB[I];
  switch(I)
  {
#include "CodesXCB.h"
    default:
		if(R->TrapBadOps) {}
  }
#undef XX
}

static void CodesED(register Z80 *R)
{
  register byte I;
  register pair J;

  I=RdZ80(R->PC.W++);
  R->ICount-=CyclesED[I];
  switch(I)
  {
#include "CodesED.h"
    case PFX_ED:
      R->PC.W--;break;
    default:
		if(R->TrapBadOps) {}
  }
}

static void CodesDD(register Z80 *R)
{
  register byte I;
  register pair J;

#define XX IX
  I=RdZ80(R->PC.W++);
  R->ICount-=CyclesXX[I];
  switch(I)
  {
#include "CodesXX.h"
    case PFX_FD:
    case PFX_DD:
      R->PC.W--;break;
    case PFX_CB:
      CodesDDCB(R);break;
    case HALT:
      R->PC.W--;R->IFF|=0x80;R->ICount=0;break;
    default:
		if(R->TrapBadOps) {}
  }
#undef XX
}

static void CodesFD(register Z80 *R)
{
  register byte I;
  register pair J;

#define XX IY
  I=RdZ80(R->PC.W++);
  R->ICount-=CyclesXX[I];
  switch(I)
  {
#include "CodesXX.h"
    case PFX_FD:
    case PFX_DD:
      R->PC.W--;break;
    case PFX_CB:
      CodesFDCB(R);break;
    case HALT:
      R->PC.W--;R->IFF|=0x80;R->ICount=0;break;
  }
#undef XX
}

/** ResetZ80() ***********************************************/
/** This function can be used to reset the register struct  **/
/** before starting execution with Z80(). It sets the       **/
/** registers to their supposed initial values.             **/
/*************************************************************/
void ResetZ80(Z80 *R)
{
  R->PC.W=0x0000;R->SP.W=0xF000;
  R->AF.W=R->BC.W=R->DE.W=R->HL.W=0x0000;
  R->AF1.W=R->BC1.W=R->DE1.W=R->HL1.W=0x0000;
  R->IX.W=R->IY.W=0x0000;
  R->I=0x00;R->IFF=0x00;
  R->ICount=R->IPeriod;
  R->IRequest=INT_NONE;
}

/** ExecZ80() ************************************************/
/** This function will execute a single Z80 opcode. It will **/
/** then return next PC, and current register values in R.  **/
/*************************************************************/
word ExecZ80(Z80 *R)
{
  register byte I;
  register pair J;

	I=RdZ80(R->PC.W++);
	R->ICount-=Cycles[I];
	switch(I)
	{
#include "Codes.h"
    case PFX_CB: CodesCB(R);break;
    case PFX_ED: CodesED(R);break;
    case PFX_FD: CodesFD(R);break;
    case PFX_DD: CodesDD(R);break;
	}
	LoopZ80(R);
  /* We are done */
  return(R->PC.W);
}

/** IntZ80() *************************************************/
/** This function will generate interrupt of given vector.  **/
/*************************************************************/
void IntZ80(Z80 *R,word Vector)
{
  if((R->IFF&0x01)||(Vector==INT_NMI))
  {
    /* Experimental V Shouldn't disable all interrupts? */
    R->IFF=(R->IFF&0x9E)|((R->IFF&0x01)<<6);
    if(R->IFF&0x80) { R->PC.W++;R->IFF&=0x7F; }
    M_PUSH(PC);

    if(Vector==INT_NMI) R->PC.W=INT_NMI;
    else
      if(R->IFF&0x04)
      { 
        Vector=(Vector&0xFF)|((word)(R->I)<<8);
        R->PC.B.l=RdZ80(Vector++);
        R->PC.B.h=RdZ80(Vector);
      }
      else
        if(R->IFF&0x02) R->PC.W=INT_IRQ;
        else R->PC.W=Vector;
  }
}

/** RunZ80() *************************************************/
/** This function will run Z80 code until an LoopZ80() call **/
/** returns INT_QUIT. It will return the PC at which        **/
/** emulation stopped, and current register values in R.    **/
/*************************************************************/
word RunZ80(Z80 *R)
{
  word PC;
 
  while(R->ICount >= 0) 
  {
	  PC = ExecZ80(R);
	  if (R->Trace == 1)
		return(R->PC.W);
  }
  R->ICount +=R->IPeriod;    

  return(R->PC.W);

}


static int PrintHex;          /* Print hexadecimal codes      */
static unsigned long Counter; /* Address counter              */

/*
static int DAsm(char *S,byte *A);
    /* This function will disassemble a single command and    */
    /* return the number of bytes disassembled.               */

static char *Mnemonics[256] =
{
  "NOP","LD BC,#h","LD (BC),A","INC BC","INC B","DEC B","LD B,*h","RLCA",
  "EX AF,AF'","ADD HL,BC","LD A,(BC)","DEC BC","INC C","DEC C","LD C,*h","RRCA",
  "DJNZ @h","LD DE,#h","LD (DE),A","INC DE","INC D","DEC D","LD D,*h","RLA",
  "JR @h","ADD HL,DE","LD A,(DE)","DEC DE","INC E","DEC E","LD E,*h","RRA",
  "JR NZ,@h","LD HL,#h","LD (#h),HL","INC HL","INC H","DEC H","LD H,*h","DAA",
  "JR Z,@h","ADD HL,HL","LD HL,(#h)","DEC HL","INC L","DEC L","LD L,*h","CPL",
  "JR NC,@h","LD SP,#h","LD (#h),A","INC SP","INC (HL)","DEC (HL)","LD (HL),*h","SCF",
  "JR C,@h","ADD HL,SP","LD A,(#h)","DEC SP","INC A","DEC A","LD A,*h","CCF",
  "LD B,B","LD B,C","LD B,D","LD B,E","LD B,H","LD B,L","LD B,(HL)","LD B,A",
  "LD C,B","LD C,C","LD C,D","LD C,E","LD C,H","LD C,L","LD C,(HL)","LD C,A",
  "LD D,B","LD D,C","LD D,D","LD D,E","LD D,H","LD D,L","LD D,(HL)","LD D,A",
  "LD E,B","LD E,C","LD E,D","LD E,E","LD E,H","LD E,L","LD E,(HL)","LD E,A",
  "LD H,B","LD H,C","LD H,D","LD H,E","LD H,H","LD H,L","LD H,(HL)","LD H,A",
  "LD L,B","LD L,C","LD L,D","LD L,E","LD L,H","LD L,L","LD L,(HL)","LD L,A",
  "LD (HL),B","LD (HL),C","LD (HL),D","LD (HL),E","LD (HL),H","LD (HL),L","HALT","LD (HL),A",
  "LD A,B","LD A,C","LD A,D","LD A,E","LD A,H","LD A,L","LD A,(HL)","LD A,A",
  "ADD B","ADD C","ADD D","ADD E","ADD H","ADD L","ADD (HL)","ADD A",
  "ADC B","ADC C","ADC D","ADC E","ADC H","ADC L","ADC (HL)","ADC A",
  "SUB B","SUB C","SUB D","SUB E","SUB H","SUB L","SUB (HL)","SUB A",
  "SBC B","SBC C","SBC D","SBC E","SBC H","SBC L","SBC (HL)","SBC A",
  "AND B","AND C","AND D","AND E","AND H","AND L","AND (HL)","AND A",
  "XOR B","XOR C","XOR D","XOR E","XOR H","XOR L","XOR (HL)","XOR A",
  "OR B","OR C","OR D","OR E","OR H","OR L","OR (HL)","OR A",
  "CP B","CP C","CP D","CP E","CP H","CP L","CP (HL)","CP A",
  "RET NZ","POP BC","JP NZ,#h","JP #h","CALL NZ,#h","PUSH BC","ADD *h","RST 00h",
  "RET Z","RET","JP Z,#h","PFX_CB","CALL Z,#h","CALL #h","ADC *h","RST 08h",
  "RET NC","POP DE","JP NC,#h","OUTA (*h)","CALL NC,#h","PUSH DE","SUB *h","RST 10h",
  "RET C","EXX","JP C,#h","INA (*h)","CALL C,#h","PFX_DD","SBC *h","RST 18h",
  "RET PO","POP HL","JP PO,#h","EX HL,(SP)","CALL PO,#h","PUSH HL","AND *h","RST 20h",
  "RET PE","LD PC,HL","JP PE,#h","EX DE,HL","CALL PE,#h","PFX_ED","XOR *h","RST 28h",
  "RET P","POP AF","JP P,#h","DI","CALL P,#h","PUSH AF","OR *h","RST 30h",
  "RET M","LD SP,HL","JP M,#h","EI","CALL M,#h","PFX_FD","CP *h","RST 38h"
};

static char *MnemonicsCB[256] =
{
  "RLC B","RLC C","RLC D","RLC E","RLC H","RLC L","RLC (HL)","RLC A",
  "RRC B","RRC C","RRC D","RRC E","RRC H","RRC L","RRC (HL)","RRC A",
  "RL B","RL C","RL D","RL E","RL H","RL L","RL (HL)","RL A",
  "RR B","RR C","RR D","RR E","RR H","RR L","RR (HL)","RR A",
  "SLA B","SLA C","SLA D","SLA E","SLA H","SLA L","SLA (HL)","SLA A",
  "SRA B","SRA C","SRA D","SRA E","SRA H","SRA L","SRA (HL)","SRA A",
  "SLL B","SLL C","SLL D","SLL E","SLL H","SLL L","SLL (HL)","SLL A",
  "SRL B","SRL C","SRL D","SRL E","SRL H","SRL L","SRL (HL)","SRL A",
  "BIT 0,B","BIT 0,C","BIT 0,D","BIT 0,E","BIT 0,H","BIT 0,L","BIT 0,(HL)","BIT 0,A",
  "BIT 1,B","BIT 1,C","BIT 1,D","BIT 1,E","BIT 1,H","BIT 1,L","BIT 1,(HL)","BIT 1,A",
  "BIT 2,B","BIT 2,C","BIT 2,D","BIT 2,E","BIT 2,H","BIT 2,L","BIT 2,(HL)","BIT 2,A",
  "BIT 3,B","BIT 3,C","BIT 3,D","BIT 3,E","BIT 3,H","BIT 3,L","BIT 3,(HL)","BIT 3,A",
  "BIT 4,B","BIT 4,C","BIT 4,D","BIT 4,E","BIT 4,H","BIT 4,L","BIT 4,(HL)","BIT 4,A",
  "BIT 5,B","BIT 5,C","BIT 5,D","BIT 5,E","BIT 5,H","BIT 5,L","BIT 5,(HL)","BIT 5,A",
  "BIT 6,B","BIT 6,C","BIT 6,D","BIT 6,E","BIT 6,H","BIT 6,L","BIT 6,(HL)","BIT 6,A",
  "BIT 7,B","BIT 7,C","BIT 7,D","BIT 7,E","BIT 7,H","BIT 7,L","BIT 7,(HL)","BIT 7,A",
  "RES 0,B","RES 0,C","RES 0,D","RES 0,E","RES 0,H","RES 0,L","RES 0,(HL)","RES 0,A",
  "RES 1,B","RES 1,C","RES 1,D","RES 1,E","RES 1,H","RES 1,L","RES 1,(HL)","RES 1,A",
  "RES 2,B","RES 2,C","RES 2,D","RES 2,E","RES 2,H","RES 2,L","RES 2,(HL)","RES 2,A",
  "RES 3,B","RES 3,C","RES 3,D","RES 3,E","RES 3,H","RES 3,L","RES 3,(HL)","RES 3,A",
  "RES 4,B","RES 4,C","RES 4,D","RES 4,E","RES 4,H","RES 4,L","RES 4,(HL)","RES 4,A",
  "RES 5,B","RES 5,C","RES 5,D","RES 5,E","RES 5,H","RES 5,L","RES 5,(HL)","RES 5,A",
  "RES 6,B","RES 6,C","RES 6,D","RES 6,E","RES 6,H","RES 6,L","RES 6,(HL)","RES 6,A",
  "RES 7,B","RES 7,C","RES 7,D","RES 7,E","RES 7,H","RES 7,L","RES 7,(HL)","RES 7,A",
  "SET 0,B","SET 0,C","SET 0,D","SET 0,E","SET 0,H","SET 0,L","SET 0,(HL)","SET 0,A",
  "SET 1,B","SET 1,C","SET 1,D","SET 1,E","SET 1,H","SET 1,L","SET 1,(HL)","SET 1,A",
  "SET 2,B","SET 2,C","SET 2,D","SET 2,E","SET 2,H","SET 2,L","SET 2,(HL)","SET 2,A",
  "SET 3,B","SET 3,C","SET 3,D","SET 3,E","SET 3,H","SET 3,L","SET 3,(HL)","SET 3,A",
  "SET 4,B","SET 4,C","SET 4,D","SET 4,E","SET 4,H","SET 4,L","SET 4,(HL)","SET 4,A",
  "SET 5,B","SET 5,C","SET 5,D","SET 5,E","SET 5,H","SET 5,L","SET 5,(HL)","SET 5,A",
  "SET 6,B","SET 6,C","SET 6,D","SET 6,E","SET 6,H","SET 6,L","SET 6,(HL)","SET 6,A",
  "SET 7,B","SET 7,C","SET 7,D","SET 7,E","SET 7,H","SET 7,L","SET 7,(HL)","SET 7,A"
};

static char *MnemonicsED[256] =
{
  "DB EDh,00h","DB EDh,01h","DB EDh,02h","DB EDh,03h",
  "DB EDh,04h","DB EDh,05h","DB EDh,06h","DB EDh,07h",
  "DB EDh,08h","DB EDh,09h","DB EDh,0Ah","DB EDh,0Bh",
  "DB EDh,0Ch","DB EDh,0Dh","DB EDh,0Eh","DB EDh,0Fh",
  "DB EDh,10h","DB EDh,11h","DB EDh,12h","DB EDh,13h",
  "DB EDh,14h","DB EDh,15h","DB EDh,16h","DB EDh,17h",
  "DB EDh,18h","DB EDh,19h","DB EDh,1Ah","DB EDh,1Bh",
  "DB EDh,1Ch","DB EDh,1Dh","DB EDh,1Eh","DB EDh,1Fh",
  "DB EDh,20h","DB EDh,21h","DB EDh,22h","DB EDh,23h",
  "DB EDh,24h","DB EDh,25h","DB EDh,26h","DB EDh,27h",
  "DB EDh,28h","DB EDh,29h","DB EDh,2Ah","DB EDh,2Bh",
  "DB EDh,2Ch","DB EDh,2Dh","DB EDh,2Eh","DB EDh,2Fh",
  "DB EDh,30h","DB EDh,31h","DB EDh,32h","DB EDh,33h",
  "DB EDh,34h","DB EDh,35h","DB EDh,36h","DB EDh,37h",
  "DB EDh,38h","DB EDh,39h","DB EDh,3Ah","DB EDh,3Bh",
  "DB EDh,3Ch","DB EDh,3Dh","DB EDh,3Eh","DB EDh,3Fh",
  "IN B,(C)","OUT (C),B","SBC HL,BC","LD (#h),BC",
  "NEG","RETN","IM 0","LD I,A",
  "IN C,(C)","OUT (C),C","ADC HL,BC","LD BC,(#h)",
  "DB EDh,4Ch","RETI","DB EDh,4Eh","LD R,A",
  "IN D,(C)","OUT (C),D","SBC HL,DE","LD (#h),DE",
  "DB EDh,54h","DB EDh,55h","IM 1","LD A,I",
  "IN E,(C)","OUT (C),E","ADC HL,DE","LD DE,(#h)",
  "DB EDh,5Ch","DB EDh,5Dh","IM 2","LD A,R",
  "IN H,(C)","OUT (C),H","SBC HL,HL","LD (#h),HL",
  "DB EDh,64h","DB EDh,65h","DB EDh,66h","RRD",
  "IN L,(C)","OUT (C),L","ADC HL,HL","LD HL,(#h)",
  "DB EDh,6Ch","DB EDh,6Dh","DB EDh,6Eh","RLD",
  "IN F,(C)","DB EDh,71h","SBC HL,SP","LD (#h),SP",
  "DB EDh,74h","DB EDh,75h","DB EDh,76h","DB EDh,77h",
  "IN A,(C)","OUT (C),A","ADC HL,SP","LD SP,(#h)",
  "DB EDh,7Ch","DB EDh,7Dh","DB EDh,7Eh","DB EDh,7Fh",
  "DB EDh,80h","DB EDh,81h","DB EDh,82h","DB EDh,83h",
  "DB EDh,84h","DB EDh,85h","DB EDh,86h","DB EDh,87h",
  "DB EDh,88h","DB EDh,89h","DB EDh,8Ah","DB EDh,8Bh",
  "DB EDh,8Ch","DB EDh,8Dh","DB EDh,8Eh","DB EDh,8Fh",
  "DB EDh,90h","DB EDh,91h","DB EDh,92h","DB EDh,93h",
  "DB EDh,94h","DB EDh,95h","DB EDh,96h","DB EDh,97h",
  "DB EDh,98h","DB EDh,99h","DB EDh,9Ah","DB EDh,9Bh",
  "DB EDh,9Ch","DB EDh,9Dh","DB EDh,9Eh","DB EDh,9Fh",
  "LDI","CPI","INI","OUTI",
  "DB EDh,A4h","DB EDh,A5h","DB EDh,A6h","DB EDh,A7h",
  "LDD","CPD","IND","OUTD",
  "DB EDh,ACh","DB EDh,ADh","DB EDh,AEh","DB EDh,AFh",
  "LDIR","CPIR","INIR","OTIR",
  "DB EDh,B4h","DB EDh,B5h","DB EDh,B6h","DB EDh,B7h",
  "LDDR","CPDR","INDR","OTDR",
  "DB EDh,BCh","DB EDh,BDh","DB EDh,BEh","DB EDh,BFh",
  "DB EDh,C0h","DB EDh,C1h","DB EDh,C2h","DB EDh,C3h",
  "DB EDh,C4h","DB EDh,C5h","DB EDh,C6h","DB EDh,C7h",
  "DB EDh,C8h","DB EDh,C9h","DB EDh,CAh","DB EDh,CBh",
  "DB EDh,CCh","DB EDh,CDh","DB EDh,CEh","DB EDh,CFh",
  "DB EDh,D0h","DB EDh,D1h","DB EDh,D2h","DB EDh,D3h",
  "DB EDh,D4h","DB EDh,D5h","DB EDh,D6h","DB EDh,D7h",
  "DB EDh,D8h","DB EDh,D9h","DB EDh,DAh","DB EDh,DBh",
  "DB EDh,DCh","DB EDh,DDh","DB EDh,DEh","DB EDh,DFh",
  "DB EDh,E0h","DB EDh,E1h","DB EDh,E2h","DB EDh,E3h",
  "DB EDh,E4h","DB EDh,E5h","DB EDh,E6h","DB EDh,E7h",
  "DB EDh,E8h","DB EDh,E9h","DB EDh,EAh","DB EDh,EBh",
  "DB EDh,ECh","DB EDh,EDh","DB EDh,EEh","DB EDh,EFh",
  "DB EDh,F0h","DB EDh,F1h","DB EDh,F2h","DB EDh,F3h",
  "DB EDh,F4h","DB EDh,F5h","DB EDh,F6h","DB EDh,F7h",
  "DB EDh,F8h","DB EDh,F9h","DB EDh,FAh","DB EDh,FBh",
  "DB EDh,FCh","DB EDh,FDh","DB EDh,FEh","DB EDh,FFh"
};

static char *MnemonicsXX[256] =
{
  "NOP","LD BC,#h","LD (BC),A","INC BC","INC B","DEC B","LD B,*h","RLCA",
  "EX AF,AF'","ADD I%,BC","LD A,(BC)","DEC BC","INC C","DEC C","LD C,*h","RRCA",
  "DJNZ @h","LD DE,#h","LD (DE),A","INC DE","INC D","DEC D","LD D,*h","RLA",
  "JR @h","ADD I%,DE","LD A,(DE)","DEC DE","INC E","DEC E","LD E,*h","RRA",
  "JR NZ,@h","LD I%,#h","LD (#h),I%","INC I%","INC I%h","DEC I%h","LD I%h,*h","DAA",
  "JR Z,@h","ADD I%,I%","LD I%,(#h)","DEC I%","INC I%l","DEC I%l","LD I%l,*h","CPL",
  "JR NC,@h","LD SP,#h","LD (#h),A","INC SP","INC (I%+^h)","DEC (I%+^h)","LD (I%+^h),*h","SCF",
  "JR C,@h","ADD I%,SP","LD A,(#h)","DEC SP","INC A","DEC A","LD A,*h","CCF",
  "LD B,B","LD B,C","LD B,D","LD B,E","LD B,I%h","LD B,I%l","LD B,(I%+^h)","LD B,A",
  "LD C,B","LD C,C","LD C,D","LD C,E","LD C,I%h","LD C,I%l","LD C,(I%+^h)","LD C,A",
  "LD D,B","LD D,C","LD D,D","LD D,E","LD D,I%h","LD D,I%l","LD D,(I%+^h)","LD D,A",
  "LD E,B","LD E,C","LD E,D","LD E,E","LD E,I%h","LD E,I%l","LD E,(I%+^h)","LD E,A",
  "LD I%h,B","LD I%h,C","LD I%h,D","LD I%h,E","LD I%h,I%h","LD I%h,I%l","LD H,(I%+^h)","LD I%h,A",
  "LD I%l,B","LD I%l,C","LD I%l,D","LD I%l,E","LD I%l,I%h","LD I%l,I%l","LD L,(I%+^h)","LD I%l,A",
  "LD (I%+^h),B","LD (I%+^h),C","LD (I%+^h),D","LD (I%+^h),E","LD (I%+^h),H","LD (I%+^h),L","HALT","LD (I%+^h),A",
  "LD A,B","LD A,C","LD A,D","LD A,E","LD A,I%h","LD A,I%l","LD A,(I%+^h)","LD A,A",
  "ADD B","ADD C","ADD D","ADD E","ADD I%h","ADD I%l","ADD (I%+^h)","ADD A",
  "ADC B","ADC C","ADC D","ADC E","ADC I%h","ADC I%l","ADC (I%+^h)","ADC,A",
  "SUB B","SUB C","SUB D","SUB E","SUB I%h","SUB I%l","SUB (I%+^h)","SUB A",
  "SBC B","SBC C","SBC D","SBC E","SBC I%h","SBC I%l","SBC (I%+^h)","SBC A",
  "AND B","AND C","AND D","AND E","AND I%h","AND I%l","AND (I%+^h)","AND A",
  "XOR B","XOR C","XOR D","XOR E","XOR I%h","XOR I%l","XOR (I%+^h)","XOR A",
  "OR B","OR C","OR D","OR E","OR I%h","OR I%l","OR (I%+^h)","OR A",
  "CP B","CP C","CP D","CP E","CP I%h","CP I%l","CP (I%+^h)","CP A",
  "RET NZ","POP BC","JP NZ,#h","JP #h","CALL NZ,#h","PUSH BC","ADD *h","RST 00h",
  "RET Z","RET","JP Z,#h","PFX_CB","CALL Z,#h","CALL #h","ADC *h","RST 08h",
  "RET NC","POP DE","JP NC,#h","OUTA (*h)","CALL NC,#h","PUSH DE","SUB *h","RST 10h",
  "RET C","EXX","JP C,#h","INA (*h)","CALL C,#h","PFX_DD","SBC *h","RST 18h",
  "RET PO","POP I%","JP PO,#h","EX I%,(SP)","CALL PO,#h","PUSH I%","AND *h","RST 20h",
  "RET PE","LD PC,I%","JP PE,#h","EX DE,I%","CALL PE,#h","PFX_ED","XOR *h","RST 28h",
  "RET P","POP AF","JP P,#h","DI","CALL P,#h","PUSH AF","OR *h","RST 30h",
  "RET M","LD SP,I%","JP M,#h","EI","CALL M,#h","PFX_FD","CP *h","RST 38h"
};

static char *MnemonicsXCB[256] =
{
  "RLC B","RLC C","RLC D","RLC E","RLC H","RLC L","RLC (I%@h)","RLC A",
  "RRC B","RRC C","RRC D","RRC E","RRC H","RRC L","RRC (I%@h)","RRC A",
  "RL B","RL C","RL D","RL E","RL H","RL L","RL (I%@h)","RL A",
  "RR B","RR C","RR D","RR E","RR H","RR L","RR (I%@h)","RR A",
  "SLA B","SLA C","SLA D","SLA E","SLA H","SLA L","SLA (I%@h)","SLA A",
  "SRA B","SRA C","SRA D","SRA E","SRA H","SRA L","SRA (I%@h)","SRA A",
  "SLL B","SLL C","SLL D","SLL E","SLL H","SLL L","SLL (I%@h)","SLL A",
  "SRL B","SRL C","SRL D","SRL E","SRL H","SRL L","SRL (I%@h)","SRL A",
  "BIT 0,B","BIT 0,C","BIT 0,D","BIT 0,E","BIT 0,H","BIT 0,L","BIT 0,(I%@h)","BIT 0,A",
  "BIT 1,B","BIT 1,C","BIT 1,D","BIT 1,E","BIT 1,H","BIT 1,L","BIT 1,(I%@h)","BIT 1,A",
  "BIT 2,B","BIT 2,C","BIT 2,D","BIT 2,E","BIT 2,H","BIT 2,L","BIT 2,(I%@h)","BIT 2,A",
  "BIT 3,B","BIT 3,C","BIT 3,D","BIT 3,E","BIT 3,H","BIT 3,L","BIT 3,(I%@h)","BIT 3,A",
  "BIT 4,B","BIT 4,C","BIT 4,D","BIT 4,E","BIT 4,H","BIT 4,L","BIT 4,(I%@h)","BIT 4,A",
  "BIT 5,B","BIT 5,C","BIT 5,D","BIT 5,E","BIT 5,H","BIT 5,L","BIT 5,(I%@h)","BIT 5,A",
  "BIT 6,B","BIT 6,C","BIT 6,D","BIT 6,E","BIT 6,H","BIT 6,L","BIT 6,(I%@h)","BIT 6,A",
  "BIT 7,B","BIT 7,C","BIT 7,D","BIT 7,E","BIT 7,H","BIT 7,L","BIT 7,(I%@h)","BIT 7,A",
  "RES 0,B","RES 0,C","RES 0,D","RES 0,E","RES 0,H","RES 0,L","RES 0,(I%@h)","RES 0,A",
  "RES 1,B","RES 1,C","RES 1,D","RES 1,E","RES 1,H","RES 1,L","RES 1,(I%@h)","RES 1,A",
  "RES 2,B","RES 2,C","RES 2,D","RES 2,E","RES 2,H","RES 2,L","RES 2,(I%@h)","RES 2,A",
  "RES 3,B","RES 3,C","RES 3,D","RES 3,E","RES 3,H","RES 3,L","RES 3,(I%@h)","RES 3,A",
  "RES 4,B","RES 4,C","RES 4,D","RES 4,E","RES 4,H","RES 4,L","RES 4,(I%@h)","RES 4,A",
  "RES 5,B","RES 5,C","RES 5,D","RES 5,E","RES 5,H","RES 5,L","RES 5,(I%@h)","RES 5,A",
  "RES 6,B","RES 6,C","RES 6,D","RES 6,E","RES 6,H","RES 6,L","RES 6,(I%@h)","RES 6,A",
  "RES 7,B","RES 7,C","RES 7,D","RES 7,E","RES 7,H","RES 7,L","RES 7,(I%@h)","RES 7,A",
  "SET 0,B","SET 0,C","SET 0,D","SET 0,E","SET 0,H","SET 0,L","SET 0,(I%@h)","SET 0,A",
  "SET 1,B","SET 1,C","SET 1,D","SET 1,E","SET 1,H","SET 1,L","SET 1,(I%@h)","SET 1,A",
  "SET 2,B","SET 2,C","SET 2,D","SET 2,E","SET 2,H","SET 2,L","SET 2,(I%@h)","SET 2,A",
  "SET 3,B","SET 3,C","SET 3,D","SET 3,E","SET 3,H","SET 3,L","SET 3,(I%@h)","SET 3,A",
  "SET 4,B","SET 4,C","SET 4,D","SET 4,E","SET 4,H","SET 4,L","SET 4,(I%@h)","SET 4,A",
  "SET 5,B","SET 5,C","SET 5,D","SET 5,E","SET 5,H","SET 5,L","SET 5,(I%@h)","SET 5,A",
  "SET 6,B","SET 6,C","SET 6,D","SET 6,E","SET 6,H","SET 6,L","SET 6,(I%@h)","SET 6,A",
  "SET 7,B","SET 7,C","SET 7,D","SET 7,E","SET 7,H","SET 7,L","SET 7,(I%@h)","SET 7,A"
};

/** DAsm() ****************************************************/
/** This function will disassemble a single command and      **/
/** return the number of bytes disassembled.                 **/
/**************************************************************/
int DAsm(char *S,byte *A)
{
  char R[128],H[10],C,*T,*P;
  byte *B,J,Offset;

  B=A;C='\0';J=0;

  switch(*B)
  {
    case 0xCB: B++;T=MnemonicsCB[*B++];break;
    case 0xED: B++;T=MnemonicsED[*B++];break;
    case 0xDD: B++;C='X';
               if(*B!=0xCB) T=MnemonicsXX[*B++];
               else { B++;Offset=*B++;J=1;T=MnemonicsXCB[*B++]; }
               break;
    case 0xFD: B++;C='Y';
               if(*B!=0xCB) T=MnemonicsXX[*B++];
               else { B++;Offset=*B++;J=1;T=MnemonicsXCB[*B++]; }
               break;
    default:   T=Mnemonics[*B++];
  }

  if(P=strchr(T,'^'))
  {
    strncpy(R,T,P-T);R[P-T]='\0';
    sprintf(H,"%02X",*B++);
    strcat(R,H);strcat(R,P+1);
  }
  else strcpy(R,T);
  if(P=strchr(R,'%')) *P=C;

  if(P=strchr(R,'*'))
  {
    strncpy(S,R,P-R);S[P-R]='\0';
    sprintf(H,"%02X",*B++);
    strcat(S,H);strcat(S,P+1);
  }
  else
    if(P=strchr(R,'@'))
    {
      strncpy(S,R,P-R);S[P-R]='\0';
      if(!J) Offset=*B++;
      strcat(S,Offset&0x80? "-":"+");
      J=Offset&0x80? 256-Offset:Offset;
      sprintf(H,"%02X",J);
      strcat(S,H);strcat(S,P+1);
    }
    else
      if(P=strchr(R,'#'))
      {
        strncpy(S,R,P-R);S[P-R]='\0';
        sprintf(H,"%04X",B[0]+256*B[1]);
        strcat(S,H);strcat(S,P+1);
        B+=2;
      }
      else strcpy(S,R);

  return(B-A);
}
