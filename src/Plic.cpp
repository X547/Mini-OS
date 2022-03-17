#include "Plic.h"
#include "RiscV.h"


template<typename Type>
inline static void SetBit(Type &val, int bit) {val |= Type(1) << bit;}

template<typename Type>
inline static void ClearBit(Type &val, int bit) {val &= ~(Type(1) << bit);}

template<typename Type>
inline static void InvertBit(Type &val, int bit) {val ^= Type(1) << bit;}

template<typename Type>
inline static void SetBitTo(Type &val, int bit, bool isSet) {val ^= ((isSet? -1: 0) ^ val) & (Type(1) << bit);}

template<typename Type>
inline static bool IsBitSet(Type val, int bit) {return (val & (Type(1) << bit)) != 0;}


PlicRegs *volatile gPlicRegs;


namespace Plic {


void Init()
{
	gPlicRegs->priority[modeS + 0] = 0;
}

void EnableIrq(uint32_t irq, bool isEnabled)
{
	gPlicRegs->priority[irq] = isEnabled? 1: 0;
	SetBitTo(gPlicRegs->enable[modeS + 0][irq / 32], irq % 32, isEnabled);
}

uint32_t Claim()
{
	return gPlicRegs->contexts[modeS + 0].claimAndComplete;
}

void Complete(uint32_t irq)
{
	gPlicRegs->contexts[modeS + 0].claimAndComplete = irq;
}


};
