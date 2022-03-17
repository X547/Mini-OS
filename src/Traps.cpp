#include <stdlib.h>
#include "Traps.h"
#include "RiscV.h"
#include "Graphics.h"
#include "Modules.h"
#include "Timers.h"
#include "Virtio.h"
#include "Plic.h"
#include "Htif.h"


namespace Traps {


//#pragma mark Debug output

void WriteMode(int mode)
{ 
	switch (mode) {
	case modeU: WriteString("u"); break;
	case modeS: WriteString("s"); break;
	case modeM: WriteString("m"); break;
	default: WriteInt(mode);
	}
}

void WriteModeSet(uint32_t val)
{
	bool first = true;
	WriteString("{");
	for (int i = 0; i < 32; i++) {
		if (((1LL << i) & val) != 0) {
			if (first) first = false; else WriteString(", ");
			WriteMode(i);
		}
	}
	WriteString("}");
}

void WriteMstatus(uint64_t val)
{
	WriteString("(");
	WriteString("ie: ");
	WriteModeSet(val & 0xf);
	WriteString(", pie: ");
	WriteModeSet((val >> 4) & 0xf);
	WriteString(", spp: "); WriteMode((val >> 8) % 2);
	WriteString(", mpp: "); WriteMode((val >> 11) % 4);
	WriteString(")");
}

void WriteSstatus(uint64_t val)
{
	WriteString("(");
	WriteString("ie: ");
	WriteModeSet(val & 0x3);
	WriteString(", pie: ");
	WriteModeSet((val >> 4) & 0x3);
	WriteString(", spp: "); WriteMode((val >> 8) % 2);
	WriteString(")");
}

void WriteInterrupt(uint64_t val)
{
	switch (val) {
	case 0 + modeU: WriteString("uSoft"); break;
	case 0 + modeS: WriteString("sSoft"); break;
	case 0 + modeM: WriteString("mSoft"); break;
	case 4 + modeU: WriteString("uTimer"); break;
	case 4 + modeS: WriteString("sTimer"); break;
	case 4 + modeM: WriteString("mTimer"); break;
	case 8 + modeU: WriteString("uExtern"); break;
	case 8 + modeS: WriteString("sExtern"); break;
	case 8 + modeM: WriteString("mExtern"); break;
	default: WriteInt(val);
	}
}

void WriteInterruptSet(uint64_t val)
{
	bool first = true;
	WriteString("{");
	for (int i = 0; i < 64; i++) {
		if (((1LL << i) & val) != 0) {
			if (first) first = false; else WriteString(", ");
			WriteInterrupt(i);
		}
	}
	WriteString("}");
}

void WriteCause(uint64_t cause)
{
	if ((cause >> 63) == 0) {
		WriteString("exception ");
		switch (cause & ~(1LL << 63)) {
			case causeExecMisalign: WriteString("execMisalign"); break;
			case causeExecAccessFault: WriteString("execAccessFault"); break;
			case causeIllegalInst: WriteString("illegalInst"); break;
			case causeBreakpoint: WriteString("breakpoint"); break;
			case causeLoadMisalign: WriteString("loadMisalign"); break;
			case causeLoadAccessFault: WriteString("loadAccessFault"); break;
			case causeStoreMisalign: WriteString("storeMisalign"); break;
			case causeStoreAccessFault: WriteString("storeAccessFault"); break;
			case causeUEcall: WriteString("uEcall"); break;
			case causeSEcall: WriteString("sEcall"); break;
			case causeMEcall: WriteString("mEcall"); break;
			case causeExecPageFault: WriteString("execPageFault"); break;
			case causeLoadPageFault: WriteString("loadPageFault"); break;
			case causeStorePageFault: WriteString("storePageFault"); break;
			default: WriteInt(cause & ~(1LL << 63));
			}
	} else {
		WriteString("interrupt "); WriteInterrupt(cause & ~(1LL << 63));
	}
}


//#pragma mark -

int64_t __attribute__((naked)) Syscall(...)
{
	__asm("addi sp, sp, -16");
	__asm("sd ra, 8(sp)");
	__asm("sd fp, 0(sp)");
	__asm("addi fp, sp, 16");

	__asm("ecall");

	__asm("ld fp, 0(sp)");
	__asm("ld ra, 8(sp)");
	__asm("addi sp, sp, 16");
	__asm("ret");
}


void MTrap(TrapFrame &tf)
{
	uint64_t cause = Mcause();
/*
	WriteString("MTrap("); WriteCause(cause); WriteString(")"); WriteLn();
	WriteTab(); WriteString("mstatus: "); WriteMstatus(Mstatus()); WriteLn();
	WriteTab(); WriteString("mepc: "); Modules::WritePC(Mepc()); WriteLn();
	WriteTab(); WriteString("mie: "); WriteInterruptSet(Mie()); WriteLn();
	WriteTab(); WriteString("mip: "); WriteInterruptSet(Mip()); WriteLn();
	WriteTab(); WriteString("sip: "); WriteInterruptSet(Sip()); WriteLn();
*/
	if ((cause & causeInterrupt) == 0) {// exception
		if (cause == causeMEcall || cause == causeSEcall) {
/*
			WriteString("Ecall"); WriteLn();
			WriteTab(); WriteString("mstatus: "); WriteMstatus(Mstatus()); WriteLn();
			Modules::StackTrace();
*/
			int64_t syscall = tf.r[10];
			switch (syscall) {
			case test1Syscall:
				WriteTab(); WriteString("test1Syscall"); WriteLn();
				break;
			case test2Syscall:
				WriteTab(); WriteString("test2Syscall(");
				WriteInt(tf.r[11]); WriteString(", ");
				WriteInt(tf.r[12]); WriteString(", ");
				WriteInt(tf.r[13]); WriteString(")"); WriteLn();
				break;
			case switchToSModeSyscall: {
				WriteTab(); WriteString("switchToSModeSyscall"); WriteLn();
				MstatusReg mstatus(Mstatus());
				mstatus.mpp = modeS;
				SetMstatus(mstatus.val); // switch to supervisor mode
				// SetMedeleg(0xffff);
				SetMideleg(0xffff);
				WriteTab(); WriteString("mstatus (2): "); WriteMstatus(Mstatus()); WriteLn();
				break;
			}
			case enableTimerSyscall: {
				bool enable = tf.r[11];
				// WriteTab(); WriteString("enableTimerSyscall("); WriteInt(enable); WriteString(")"); WriteLn();
				if (enable)
					SetMie(Mie() | (1 << 7));
				else
					SetMie(Mie() & ~(1 << 7));
				break;
			}
			default:
				WriteString("Unknown syscall"); WriteLn();
				abort();
			}
			tf.r[31] += 4;
			return;
		}
		WriteTab(); WriteString("=== PANIC ==="); WriteLn();
		WriteCause(cause); WriteLn();
		WriteString("mstatus: "); WriteMstatus(Mstatus()); WriteLn();
		WriteTab(); WriteString("mepc: "); Modules::WritePC(Mepc()); WriteLn();
		WriteTab(); WriteString("sepc: "); Modules::WritePC(Sepc()); WriteLn();
		WriteTab(); WriteString("mtval: "); Modules::WritePC(Mtval()); WriteLn();
		Modules::StackTrace();
		Shutdown();
	}
	if (cause == causeInterrupt + mTimerInt) {
		SetSip(Sip() | (1 << sTimerInt));
		SetMie(Mie() & ~(1 << 7));
/*
		uint64_t oldMstatus = Mstatus();
		uint64_t oldMepc = Mepc();
		Timers::Trap();
		SetMepc(oldMepc);
		SetMstatus(oldMstatus);
*/
		return;
	} else if (cause == causeInterrupt + sExternInt) {
		uint32_t irq = Plic::Claim();
/*
		WriteString("SExternInt"); WriteLn();
		WriteTab(); WriteString("mstatus: "); WriteMstatus(Mstatus()); WriteLn();
		WriteString("irq: "); WriteInt(irq); WriteLn();
*/
		Virtio::IrqHandler(irq);
		Plic::Complete(irq);
		return;
	}
}

void STrap(TrapFrame &tf)
{
	uint64_t cause = Scause();
/*
	WriteString("STrap("); WriteCause(cause); WriteString(")"); WriteLn();
	WriteTab(); WriteString("sstatus: "); WriteSstatus(Sstatus()); WriteLn();
	WriteTab(); WriteString("sepc: "); Modules::WritePC(Sepc()); WriteLn();
	WriteTab(); WriteString("sie: "); WriteInterruptSet(Sie()); WriteLn();
	WriteTab(); WriteString("sip: "); WriteInterruptSet(Sip()); WriteLn();
*/
	if ((cause & causeInterrupt) == 0) {
		WriteTab(); WriteString("=== PANIC ==="); WriteLn();
		WriteCause(cause); WriteLn();
		Modules::StackTrace();
		for(;;) {}
	}
	
	if (cause == causeInterrupt + sTimerInt) {
		// WriteTab(); WriteString("cause == causeSTimerInt"); WriteLn();
		SetSip(Sip() & ~(1 << sTimerInt));

		uint64_t oldSstatus = Sstatus();
		Timers::Trap();
		SetSstatus(oldSstatus);

		//Syscall(enableTimerSyscall, true);
		return;
	}
	
	if (cause == causeInterrupt + sExternInt) {
		uint32_t irq = Plic::Claim();
		// WriteTab(); WriteString("irq: "); WriteInt(irq); WriteLn();

		Virtio::IrqHandler(irq);
		Plic::Complete(irq);
	}
}


void InitPmp()
{
  SetPmpaddr0((~0L) >> 10);
  SetPmpcfg0((1 << pmpR) | (1 << pmpW) | (1 << pmpX) | (pmpMatchNapot));
}


void Init()
{
	// WriteString("mip: "); WriteSet(Mip()); WriteLn();
	SetMtvec((uint64_t)MVec);
	SetStvec((uint64_t)SVec);
/*
	WriteString("MVec: "); WriteHex((uint64_t)MVec, 8); WriteLn();
	WriteString("SVec: "); WriteHex((uint64_t)SVec, 8); WriteLn();
*/
	WriteString("mtvec: "); Modules::WritePC(Mtvec()); WriteLn();
	WriteString("stvec: "); Modules::WritePC(Stvec()); WriteLn();

	MstatusReg mstatus(Mstatus());
	mstatus.ie |= (1 << modeM) | (1 << modeS);
	SetMstatus(mstatus.val);
//	WriteString("mstatus: "); WriteMstatus(Mstatus()); WriteLn();
	SetMie(Mie() | (1 << sTimerInt) | (1 << sExternInt));
//	WriteString("mie: "); WriteInterruptSet(Mie()); WriteLn();

	InitPmp();
}

}
