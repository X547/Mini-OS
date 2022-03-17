CC = clang
CXX = clang++
AS = clang
LD = ld.lld -nostdlib

#ARCHDIR=/x86
ARCHDIR=
#GCCARCH=i586-pc-haiku
GCCARCH=x86_64-unknown-haiku
#GCCVERSION=8.3.0
GCCVERSION=11.2.0
TARGETFLAGS = -target riscv64-linux -march=rv64imafdc -mcmodel=medium -mno-relax
CXXFLAGS += -fpic -fno-omit-frame-pointer -fno-exceptions -O1 -Iheaders -Ilibc/headers -Ilibfdt/headers -Iwidgets/headers -I/boot/system/develop/tools${ARCHDIR}/lib/gcc/${GCCARCH}/${GCCVERSION}/include/c++ -I/boot/system/develop/tools${ARCHDIR}/lib/gcc/${GCCARCH}/${GCCVERSION}/include/c++/${GCCARCH}
ASFLAGS += $(TARGETFLAGS)


Startup.bin: Startup
	llvm-objcopy -O binary $< $@

Startup: objs/Entry.o objs/Startup.o objs/Vecs.o objs/Graphics.o objs/Views.o objs/Font.o objs/Pointer.o objs/IBeam.o objs/Htif.o objs/FwCfg.o objs/Plic.o objs/Virtio.o objs/Threads.o objs/Locks.o objs/Timers.o objs/Modules.o objs/Traps.o objs/VM.o \
	libc/objs/setjmp.o libc/objs/string.o libc/objs/memcpy.o libc/objs/memset.o libc/objs/malloc.o \
	libcpp/objs/New.o libcpp/objs/Rtti.o \
	libfdt/objs/fdt.o libfdt/objs/fdt_ro.o libfdt/objs/fdt_rw.o libfdt/objs/fdt_strerror.o libfdt/objs/fdt_sw.o libfdt/objs/fdt_wip.o \
	widgets/objs/TextModels.o \
	widgets/objs/Buttons.o \
	| Startup.ld
	$(LD) -T $| --export-dynamic $^ -o $@

objs/%.o: src/%.cpp
	$(CXX) $(TARGETFLAGS) $(CXXFLAGS) -MF"deps/$*.d" -MD -c $< -o $@
objs/%.o: src/%.S
	$(AS) $(ASFLAGS) -MF"deps/$*.d" -MD -c $< -o $@
libc/objs/%.o: libc/src/%.cpp
	$(CXX) $(TARGETFLAGS) $(CXXFLAGS) -MF"libc/deps/$*.d" -MD -c $< -o $@
libc/objs/%.o: libc/src/%.S
	$(AS) $(ASFLAGS) -MF"libc/deps/$*.d" -MD -c $< -o $@
libcpp/objs/%.o: libcpp/src/%.cpp
	$(CXX) $(TARGETFLAGS) $(CXXFLAGS) -MF"libcpp/deps/$*.d" -MD -c $< -o $@
libcpp/objs/%.o: libcpp/src/%.S
	$(AS) $(ASFLAGS) -MF"libc/depspp/$*.d" -MD -c $< -o $@
libfdt/objs/%.o: libfdt/src/%.cpp
	$(CXX) $(TARGETFLAGS) $(CXXFLAGS) -MF"libfdt/deps/$*.d" -MD -c $< -o $@
libfdt/objs/%.o: libfdt/src/%.S
	$(AS) $(ASFLAGS) -MF"libc/depspp/$*.d" -MD -c $< -o $@
widgets/objs/%.o: widgets/src/%.cpp
	$(CXX) $(TARGETFLAGS) $(CXXFLAGS) -MF"widgets/deps/$*.d" -MD -c $< -o $@
widgets/objs/%.o: widgets/src/%.S
	$(AS) $(ASFLAGS) -MF"libc/depspp/$*.d" -MD -c $< -o $@

run: Startup.bin
	temu -rw tinyemu.cfg

-include deps/*.d libc/deps/*.d libcpp/deps/*.d libfdt/deps/*.d widgets/deps/*.d

clean:
	rm -f objs/* deps/* libc/objs/* libc/deps/* libcpp/objs/* libcpp/deps/* libfdt/objs/* libfdt/deps/* widgets/objs/* widgets/deps/* Startup Startup.bin
