
#ifndef INST
#define INST(Mnemonic, Encoding, ...) {Op_##Mnemonic, Encoding, __VA_ARGS__},
#endif

#ifndef INSTALT
#define INSTALT INST
#endif

#define B(Bits) {Bits_Literal, sizeof(#Bits)-1, 0, 0b##Bits}
#define D {Bits_D, 1}
#define S {Bits_S, 1}
#define W {Bits_W, 1}
#define V {Bits_V, 1}
#define Z {Bits_Z, 1}

#define XXX {Bits_Data, 3, 0}
#define YYY {Bits_Data, 3, 3}
#define RM {Bits_RM, 3}
#define MOD {Bits_MOD, 2}
#define REG {Bits_REG, 3}
#define SR {Bits_SR, 2}

#define ImpW(Value) {Bits_W, 0, 0, Value}
#define ImpREG(Value) {Bits_REG, 0, 0, Value}
#define ImpMOD(Value) {Bits_MOD, 0, 0, Value}
#define ImpRM(Value) {Bits_RM, 0, 0, Value}
#define ImpD(Value) {Bits_D, 0, 0, Value}
#define ImpS(Value) {Bits_S, 0, 0, Value}
    
#define DISP {Bits_HasDisp, 0, 0, 1}
#define ADDR {Bits_HasDisp, 0, 0, 1}, {Bits_DispAlwaysW, 0, 0, 1}
#define DATA {Bits_HasData, 0, 0, 1}
#define DATA_IF_W {Bits_WMakesDataW, 0, 0, 1}
#define Flags(F) {F, 0, 0, 1}

INST(mov, {B(100010), D, W, MOD, REG, RM})
INSTALT(mov, {B(1100011), W, MOD, B(000), RM, DATA, DATA_IF_W, ImpD(0)})
INSTALT(mov, {B(1011), W, REG, DATA, DATA_IF_W, ImpD(1)})
INSTALT(mov, {B(1010000), W, ADDR, ImpREG(0), ImpMOD(0), ImpRM(0b110), ImpD(1)})
INSTALT(mov, {B(1010001), W, ADDR, ImpREG(0), ImpMOD(0), ImpRM(0b110), ImpD(0)})
INSTALT(mov, {B(100011), D, B(0), MOD, B(0), SR, RM}) // NOTE(casey): This collapses 2 entries in the 8086 table by adding an explicit D bit

#undef INST
#undef INSTALT

#undef B
#undef D
#undef S
#undef W
#undef V
#undef Z

#undef XXX
#undef YYY
#undef RM
#undef MOD
#undef REG
#undef SR

#undef ImpW
#undef ImpREG
#undef ImpMOD
#undef ImpRM
#undef ImpD
#undef ImpS

#undef DISP
#undef ADDR
#undef DATA
#undef DATA_IF_W
#undef Flags