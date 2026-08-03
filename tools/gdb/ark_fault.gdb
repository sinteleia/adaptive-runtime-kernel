define ark-print-pack-char
	if $arg0 < 10
		printf "%c", $arg0 + 48
	else
		if $arg0 < 36
			printf "%c", $arg0 + 55
		else
			if $arg0 == 36
				printf "-"
			else
				if $arg0 == 37
					printf " "
				else
					if $arg0 == 38
						printf "."
					else
						printf "?"
					end
				end
			end
		end
	end
end

define ark-print-pack16
	set $ark_pack_tmp = (unsigned int)$arg0
	set $ark_pack_c2 = $ark_pack_tmp % 40
	set $ark_pack_tmp = $ark_pack_tmp / 40
	set $ark_pack_c1 = $ark_pack_tmp % 40
	set $ark_pack_tmp = $ark_pack_tmp / 40
	set $ark_pack_c0 = $ark_pack_tmp % 40
	ark-print-pack-char $ark_pack_c0
	ark-print-pack-char $ark_pack_c1
	ark-print-pack-char $ark_pack_c2
end

define ark-fault
	set $magic = (unsigned int)ErrorTrapSnapshot.Magic
	if $magic != 0x45545250
		printf "ARK fault snapshot is not valid: Magic=0x%08x\n", $magic
		help ark-fault
	else
		set $version = (unsigned int)ErrorTrapSnapshot.Version
		set $stacked_r0_value = (unsigned int)stacked_r0
		set $stacked_r1_value = (unsigned int)stacked_r1
		set $stacked_r2_value = (unsigned int)stacked_r2
		set $stacked_r3_value = (unsigned int)stacked_r3
		set $stacked_pc_value = (unsigned int)stacked_pc
		set $stacked_lr_value = (unsigned int)stacked_lr
		set $call_site_value = ($stacked_lr_value & 0xfffffffe) - 4
		set $cfsr = (unsigned int)_CFSR.AsDW
		set $hfsr = (unsigned int)_HFSR.AsDW
		set $dfsr = (unsigned int)_DFSR.AsDW
		set $afsr = (unsigned int)_AFSR
		set $mmfar = (unsigned int)_MMFAR
		set $bfar = (unsigned int)_BFAR
		set $task_pc_value = (unsigned int)ErrorTrapSnapshot.CurrentTaskSavedPC
		set $task_lr_value = (unsigned int)ErrorTrapSnapshot.CurrentTaskSavedLR
		set $wait_caller_value = (unsigned int)ErrorTrapSnapshot.CurrentTaskWaitCallerAddress
		set $label_low = (unsigned int)ErrorTrapSnapshot.CurrentTaskLabelLow
		set $label_high = (unsigned int)ErrorTrapSnapshot.CurrentTaskLabelHigh
		set $wait_status = (unsigned int)ErrorTrapSnapshot.CurrentTaskStatus
		set $wait_kind = $wait_status & 0x7f
		set $wait_timeout = $wait_status & 0x80
		set $wait_object_value = (unsigned int)ErrorTrapSnapshot.CurrentTaskWaitObject

		printf "\nARK fault snapshot, version %u\n", $version

		if ((unsigned int)ErrorTrapSnapshot.FaultContextIsTask) != 0
			printf "Context: RTK task\n"
		else
			if ((unsigned int)ErrorTrapSnapshot.FaultContextIsISR) != 0
				printf "Context: ISR/exception number %u\n", (unsigned int)ErrorTrapSnapshot.FaultContextException
			else
				printf "Context: Thread mode outside a recognized RTK task\n"
			end
		end

		printf "\nFault:\n"
		printf "  Faulting instruction 0x%08x  ", $stacked_pc_value
		info symbol $stacked_pc_value
		info line *$stacked_pc_value
		x/i $stacked_pc_value
		printf "  API return address  0x%08x  ", $stacked_lr_value
		info symbol $stacked_lr_value
		info line *$stacked_lr_value
		printf "  API call site       0x%08x  ", $call_site_value
		info symbol $call_site_value
		info line *$call_site_value
		printf "  Fault-time args     R0=0x%08x R1=0x%08x R2=0x%08x R3=0x%08x\n", $stacked_r0_value, $stacked_r1_value, $stacked_r2_value, $stacked_r3_value
		printf "  CheckAndWaitForFlag F argument candidate: 0x%08x\n", $stacked_r0_value
		printf "  Bad address candidate 0x%08x\n", $bfar

		printf "\nTask:\n"
		printf "  CurrentTaskPtr     0x%08x\n", (unsigned int)ErrorTrapSnapshot.CurrentTaskPtr
		printf "  Label              \""
		set $label_part = $label_low & 0xffff
		ark-print-pack16 $label_part
		set $label_part = ($label_low >> 16) & 0xffff
		ark-print-pack16 $label_part
		set $label_part = $label_high & 0xffff
		ark-print-pack16 $label_part
		set $label_part = ($label_high >> 16) & 0xffff
		ark-print-pack16 $label_part
		printf "\"\n"
		printf "  Priority           0x%02x\n", (unsigned int)ErrorTrapSnapshot.CurrentTaskPriority
		printf "  Status             0x%02x\n", $wait_status
		printf "  Waiting for        "
		if $wait_kind == 0
			printf "nothing\n"
		else
			if $wait_kind == 1
				printf "task timer expiration\n"
			else
				if $wait_kind == 2
					printf "explicit ResumeTask\n"
				else
					if $wait_kind == 3
						printf "Semaphore\n"
					else
						if $wait_kind == 4
							printf "Counting semaphore\n"
						else
							if $wait_kind == 5
								printf "Flag set\n"
							else
								if $wait_kind == 6
									printf "Flag clear\n"
								else
									if $wait_kind == 7
										printf "Queue not empty\n"
									else
										if $wait_kind == 8
											printf "Binary length queue has put space\n"
										else
											if $wait_kind == 9
												printf "Free length queue has put space\n"
											else
												if $wait_kind == 10
													printf "Queue empty\n"
												else
													if $wait_kind == 11
														printf "BYTE bit set\n"
													else
														if $wait_kind == 12
															printf "BYTE bit clear\n"
														else
															if $wait_kind == 13
																printf "WORD bit set\n"
															else
																if $wait_kind == 14
																	printf "WORD bit clear\n"
																else
																	if $wait_kind == 15
																		printf "DWORD bit set\n"
																	else
																		if $wait_kind == 16
																			printf "DWORD bit clear\n"
																		else
																			printf "unknown wait type %u\n", $wait_kind
																		end
																	end
																end
															end
														end
													end
												end
											end
										end
									end
								end
							end
						end
					end
				end
			end
		end
		if $wait_timeout != 0
			printf "  Wait timeout       enabled\n"
		end
		printf "  Wait object        0x%08x (may be stale if the fault happened before wait state setup)\n", $wait_object_value
		printf "  Wait object symbol "
		info symbol $wait_object_value
		printf "  Wait param         0x%08x (mask/count/extra wait parameter, depending on wait type)\n", (unsigned int)ErrorTrapSnapshot.CurrentTaskWaitParam
		printf "  Wait caller        0x%08x", $wait_caller_value
		if $wait_caller_value != 0
			printf "  "
			info symbol $wait_caller_value
			info line *$wait_caller_value
		else
			printf "\n"
		end

		printf "\nFault cause:\n"
		if ($hfsr & 0x00000002) != 0
			printf "  HardFault: vector table read fault\n"
		end
		if ($hfsr & 0x40000000) != 0
			printf "  HardFault: escalated configurable fault\n"
		end
		if ($hfsr & 0x80000000) != 0
			printf "  HardFault: debug event\n"
		end
		if $hfsr == 0
			printf "  No HFSR cause bit is set\n"
		end

		if ($cfsr & 0x00000001) != 0
			printf "  MemManage: instruction access violation\n"
		end
		if ($cfsr & 0x00000002) != 0
			printf "  MemManage: data access violation\n"
		end
		if ($cfsr & 0x00000008) != 0
			printf "  MemManage: unstacking error during exception return\n"
		end
		if ($cfsr & 0x00000010) != 0
			printf "  MemManage: stacking error during exception entry\n"
		end
		if ($cfsr & 0x00000020) != 0
			printf "  MemManage: lazy FPU state preservation error\n"
		end
		if ($cfsr & 0x00000080) != 0
			printf "  MemManage fault address valid: 0x%08x\n", $mmfar
		end
		if ($cfsr & 0x00000100) != 0
			printf "  BusFault: instruction bus error\n"
		end
		if ($cfsr & 0x00000200) != 0
			printf "  BusFault: precise data access error\n"
		end
		if ($cfsr & 0x00000400) != 0
			printf "  BusFault: imprecise data access error; saved PC may be after the failing store\n"
		end
		if ($cfsr & 0x00000800) != 0
			printf "  BusFault: unstacking error during exception return\n"
		end
		if ($cfsr & 0x00001000) != 0
			printf "  BusFault: stacking error during exception entry\n"
		end
		if ($cfsr & 0x00002000) != 0
			printf "  BusFault: lazy FPU state preservation error\n"
		end
		if ($cfsr & 0x00008000) != 0
			printf "  BusFault address valid: 0x%08x\n", $bfar
		end
		if ($cfsr & 0x00010000) != 0
			printf "  UsageFault: undefined instruction\n"
		end
		if ($cfsr & 0x00020000) != 0
			printf "  UsageFault: invalid EPSR state\n"
		end
		if ($cfsr & 0x00040000) != 0
			printf "  UsageFault: invalid exception return\n"
		end
		if ($cfsr & 0x00080000) != 0
			printf "  UsageFault: coprocessor or FPU access not enabled\n"
		end
		if ($cfsr & 0x01000000) != 0
			printf "  UsageFault: unaligned memory access\n"
		end
		if ($cfsr & 0x02000000) != 0
			printf "  UsageFault: division by zero\n"
		end
		if $cfsr == 0
			printf "  No CFSR cause bit is set\n"
		end

		printf "\nRaw registers:\n"
		printf "  CFSR=0x%08x HFSR=0x%08x DFSR=0x%08x AFSR=0x%08x\n", $cfsr, $hfsr, $dfsr, $afsr
		printf "  MMFAR=0x%08x BFAR=0x%08x\n", $mmfar, $bfar
		printf "  EXC_RETURN=0x%08x CONTROL=0x%08x IPSR=0x%08x\n", (unsigned int)ErrorTrapSnapshot.ExcReturn, (unsigned int)ErrorTrapSnapshot.CONTROL, (unsigned int)ErrorTrapSnapshot.IPSR
		printf "  PSP=0x%08x MSP=0x%08x BASEPRI=0x%08x PRIMASK=0x%08x FAULTMASK=0x%08x\n\n", (unsigned int)ErrorTrapSnapshot.PSP, (unsigned int)ErrorTrapSnapshot.MSP, (unsigned int)ErrorTrapSnapshot.BASEPRI, (unsigned int)ErrorTrapSnapshot.PRIMASK, (unsigned int)ErrorTrapSnapshot.FAULTMASK
	end
end

document ark-fault
Print the ARK/RTK HardFault snapshot saved by ErrorTrapSnapshot.
Run this command while the target is halted in or after HardFault_GetContext.
end

printf "ARK fault command loaded. Use: ark-fault\n"
