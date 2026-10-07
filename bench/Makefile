
CA_WS ?= ~/ca2026_ws
RIPES_IMG ?= Ripes-v2.2.6-106-g5b8a616-linux-x86_64.AppImage
GUEST_BYTE_TEST ?= $(CA_WS)/minirubik/bench/guest-byte.s
IPS_TEST ?= $(CA_WS)/minirubik/bench/ips.s

# --proc <name>                 Processor model. Options: [RV32_SS, RV32_5MC_2M,
#                               RV32_5MC_1M, RV32_5S_NO_FW_HZ, RV32_5S_NO_HZ,
#                               RV32_5S_NO_FW, RV32_5S, RV32_6S_DUAL, RV64_SS,
#                               RV64_5MC, RV64_5MC_1M, RV64_5S_NO_FW_HZ,
#                               RV64_5S_NO_HZ, RV64_5S_NO_FW, RV64_5S,
#                               RV64_6S_DUAL, RV32_ISS, RV64_ISS]
PROC_MODEL ?= "RV32_ISS"

memory-ratio:
	/usr/bin/time -v $(CA_WS)/$(RIPES_IMG) --mode cli --src $(GUEST_BYTE_TEST) -t asm --proc $(PROC_MODEL)

test_ips:
	$(CA_WS)/$(RIPES_IMG) --mode cli --src $(IPS_TEST) -t asm --proc $(PROC_MODEL) --iret --exectime

ripes-info:
	$(CA_WS)/$(RIPES_IMG) --help