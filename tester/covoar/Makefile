INSTALL_DIR=../bin
CXXFLAGS=-g -Wall -O3
PROGRAMS=covoar qemu-dump-trace trace-converter configfile-test

COMMON_OBJS= app_common.o \
  ConfigFile.o \
  CoverageFactory.o \
  CoverageMap.o \
  CoverageMapBase.o \
  CoverageRanges.o \
  CoverageReaderBase.o \
  CoverageReaderQEMU.o \
  CoverageReaderRTEMS.o \
  CoverageReaderSkyeye.o \
  CoverageReaderTSIM.o \
  CoverageWriterBase.o \
  CoverageWriterRTEMS.o \
  CoverageWriterSkyeye.o \
  CoverageWriterTSIM.o \
  DesiredSymbols.o \
  ExecutableInfo.o \
  Explanations.o \
  GcovData.o \
  GcovFunctionData.o \
  ObjdumpProcessor.o \
  ReportsBase.o \
  ReportsText.o \
  ReportsHtml.o \
  SymbolTable.o \
  Target_arm.o  \
  TargetBase.o  \
  TargetFactory.o \
  Target_i386.o  \
  Target_lm32.o  \
  Target_m68k.o  \
  Target_powerpc.o \
  Target_sparc.o \

TRACECONVERTER_OBJS = \
  $(COMMON_OBJS) \
  TraceConverter.o \
  TraceList.o \
  TraceReaderBase.o \
  TraceReaderLogQEMU.o \
  TraceWriterBase.o \
  TraceWriterQEMU.o

COVOAR_OBJS = \
  $(COMMON_OBJS) \
  covoar.o

CONFIGFILE_TEST_OBJS = \
  $(COMMON_OBJS) \
  configfile_test.cc

INSTALLED= \
    ../bin/qemu-dump-trace \
    ../bin/trace-converter \
    ../bin/covoar \
    ../bin/mkExplanation

all: $(PROGRAMS) ${INSTALL_DIR} $(INSTALLED)

${INSTALL_DIR}:
	test -d ${INSTALL_DIR} || mkdir ${INSTALL_DIR}

#  INSTALLED PROGRAMS
../bin/qemu-dump-trace: qemu-dump-trace ${INSTALL_DIR}
	cp qemu-dump-trace ../bin

../bin/trace-converter: trace-converter ${INSTALL_DIR}
	cp trace-converter ../bin

../bin/covoar: covoar ${INSTALL_DIR}
	cp covoar ../bin

../bin/mkExplanation: mkExplanation ${INSTALL_DIR}
	cp mkExplanation ../bin

#  EXECUTABLES
qemu-dump-trace: qemu-dump-trace.c ${INSTALL_DIR}
	$(CXX) -o $(@) qemu-dump-trace.c

covoar: $(COVOAR_OBJS)
	$(CXX) $(CXXFLAGS) -o $(@) $(COVOAR_OBJS)

trace-converter: $(TRACECONVERTER_OBJS)
	$(CXX) $(CXXFLAGS) -o $(@) $(TRACECONVERTER_OBJS)

configfile-test: $(CONFIGFILE_TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $(@) $(CONFIGFILE_TEST_OBJS)

#  DEPENDENCIES ON SINGLE OBJECTS
app_common.o: app_common.h app_common.cc

covoar.o: covoar.cc CoverageFactory.h CoverageMap.h DesiredSymbols.h \
  ExecutableInfo.h Explanations.h ObjdumpProcessor.h ReportsBase.h

CoverageFactory.o: CoverageFactory.cc CoverageFactory.h \
  CoverageReaderBase.h CoverageReaderQEMU.h CoverageReaderRTEMS.h \
  CoverageReaderSkyeye.h CoverageReaderTSIM.h  \
  CoverageWriterBase.h CoverageWriterRTEMS.h \
  CoverageWriterSkyeye.h CoverageWriterTSIM.h
CoverageMap.o: CoverageMap.cc CoverageMap.h
CoverageMapBase.o: CoverageMapBase.cc CoverageMapBase.h
CoverageRanges.o: CoverageRanges.cc CoverageRanges.h
CoverageReaderBase.o: CoverageReaderBase.cc CoverageReaderBase.h
CoverageReaderQEMU.o: CoverageReaderQEMU.cc CoverageReaderQEMU.h \
  ExecutableInfo.h qemu-traces.h
CoverageReaderRTEMS.o: CoverageReaderRTEMS.cc CoverageReaderRTEMS.h \
  ExecutableInfo.h rtemscov_header.h
CoverageReaderSkyeye.o: CoverageReaderSkyeye.cc CoverageReaderSkyeye.h \
  ExecutableInfo.h skyeye_header.h
CoverageReaderTSIM.o: CoverageReaderTSIM.cc CoverageReaderTSIM.h \
  ExecutableInfo.h
CoverageWriterBase.o: CoverageWriterBase.cc CoverageWriterBase.h
CoverageWriterRTEMS.o: CoverageWriterRTEMS.cc CoverageWriterRTEMS.h \
  rtemscov_header.h
CoverageWriterSkyeye.o: CoverageWriterSkyeye.cc CoverageWriterSkyeye.h \
  skyeye_header.h
CoverageWriterTSIM.o: CoverageWriterTSIM.cc CoverageWriterTSIM.h
DesiredSymbols.o: DesiredSymbols.cc DesiredSymbols.h CoverageMap.h
ExecutableInfo.o: ExecutableInfo.cc ExecutableInfo.h CoverageMap.h \
  DesiredSymbols.h SymbolTable.h
Explanations.o: Explanations.cc Explanations.h
GcovData.o: GcovData.h GcovData.cc
GcovFunctionData.o: GcovFunctionData.h GcovFunctionData.cc
ObjdumpProcessor.o: ObjdumpProcessor.cc ObjdumpProcessor.h ExecutableInfo.h \
  TargetBase.h TargetFactory.h
ReportsBase.o: ReportsBase.cc ReportsBase.h CoverageRanges.h DesiredSymbols.h \
  Explanations.h ObjdumpProcessor.h
ReportsHtml.o: ReportsHtml.h ReportsText.cc
ReportsText.o: ReportsBase.h ReportsText.cc
SymbolTable.o: SymbolTable.cc SymbolTable.h
Target_arm.o: Target_arm.cc Target_arm.h TargetBase.h
TargetBase.o: TargetBase.cc TargetBase.h
TargetFactory.o: TargetFactory.cc TargetFactory.h TargetBase.h Target_arm.h \
  Target_i386.h Target_lm32.h Target_m68k.h Target_powerpc.h Target_sparc.h
Target_i386.o: Target_i386.cc Target_i386.h TargetBase.h
Target_lm32.o: Target_lm32.cc Target_lm32.h TargetBase.h
Target_m68k.o: Target_m68k.cc Target_m68k.h TargetBase.h
Target_powerpc.o: Target_powerpc.cc Target_powerpc.h TargetBase.h
Target_sparc.o: Target_sparc.cc Target_sparc.h TargetBase.h

TraceConverter.o: TraceConverter.cc TraceReaderBase.h TraceList.h
TraceList.o: TraceList.cc TraceList.h
TraceReaderBase.o: TraceReaderBase.cc TraceReaderBase.h TraceList.h
TraceReaderLogQEMU.o: TraceReaderLogQEMU.cc TraceReaderLogQEMU.h TraceReaderBase.h TraceList.h
TraceWriterBase.o: TraceWriterBase.cc TraceWriterBase.h
TraceWriterQEMU.o: TraceWriterQEMU.cc TraceWriterQEMU.h TraceWriterBase.h TraceReaderLogQEMU.h TraceList.h

clean:
	rm -rf $(PROGRAMS) *.o doxy html latex *.exe *~ warnings.log

doxygen:
	doxygen Doxyfile

install: ${INSTALLED}
