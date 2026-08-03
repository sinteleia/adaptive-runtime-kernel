define rtk-print-pack-char
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

define rtk-print-pack16
	set $rtk_pack_tmp = (unsigned int)$arg0
	set $rtk_pack_c2 = $rtk_pack_tmp % 40
	set $rtk_pack_tmp = $rtk_pack_tmp / 40
	set $rtk_pack_c1 = $rtk_pack_tmp % 40
	set $rtk_pack_tmp = $rtk_pack_tmp / 40
	set $rtk_pack_c0 = $rtk_pack_tmp % 40
	rtk-print-pack-char $rtk_pack_c0
	rtk-print-pack-char $rtk_pack_c1
	rtk-print-pack-char $rtk_pack_c2
end

define rtk-print-label64
	set $rtk_label_low = (unsigned int)$arg0
	set $rtk_label_high = (unsigned int)(((unsigned long long)$arg0) >> 32)
	set $rtk_label_part = $rtk_label_low & 0xffff
	rtk-print-pack16 $rtk_label_part
	set $rtk_label_part = ($rtk_label_low >> 16) & 0xffff
	rtk-print-pack16 $rtk_label_part
	set $rtk_label_part = $rtk_label_high & 0xffff
	rtk-print-pack16 $rtk_label_part
	set $rtk_label_part = ($rtk_label_high >> 16) & 0xffff
	rtk-print-pack16 $rtk_label_part
end

define rtk-wait-name
	set $rtk_wait_status = (unsigned int)$arg0
	set $rtk_wait_kind = $rtk_wait_status & 0x7f
	set $rtk_wait_timeout = $rtk_wait_status & 0x80
	if $rtk_wait_kind == 0
		printf "Ready"
	else
		if $rtk_wait_kind == 1
			printf "Waiting for task timer expiration"
		else
			if $rtk_wait_kind == 2
				printf "Waiting for explicit ResumeTask"
			else
				if $rtk_wait_kind == 3
					printf "Waiting for Semaphore"
				else
					if $rtk_wait_kind == 4
						printf "Waiting for Counting semaphore"
					else
						if $rtk_wait_kind == 5
							printf "Waiting for Flag set"
						else
							if $rtk_wait_kind == 6
								printf "Waiting for Flag clear"
							else
								if $rtk_wait_kind == 7
									printf "Waiting for Queue not empty"
								else
									if $rtk_wait_kind == 8
										printf "Waiting for Binary length queue put space"
									else
										if $rtk_wait_kind == 9
											printf "Waiting for Free length queue put space"
										else
											if $rtk_wait_kind == 10
												printf "Waiting for Queue empty"
											else
												if $rtk_wait_kind == 11
													printf "Waiting for BYTE bit set"
												else
													if $rtk_wait_kind == 12
														printf "Waiting for BYTE bit clear"
													else
														if $rtk_wait_kind == 13
															printf "Waiting for WORD bit set"
														else
															if $rtk_wait_kind == 14
																printf "Waiting for WORD bit clear"
															else
																if $rtk_wait_kind == 15
																	printf "Waiting for DWORD bit set"
																else
																	if $rtk_wait_kind == 16
																		printf "Waiting for DWORD bit clear"
																	else
																		printf "Unknown wait type %u", $rtk_wait_kind
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
	if $rtk_wait_timeout != 0
		printf " with timeout"
	end
end

define rtk-priority-name
	if $arg0 == 0
		printf "Critical"
	else
		if $arg0 == 1
			printf "High"
		else
			if $arg0 == 2
				printf "Medium"
			else
				if $arg0 == 3
					printf "Low"
				else
					if $arg0 == 4
						printf "Background"
					else
						if $arg0 == 5
							printf "Idle"
						else
							printf "Invalid/unknown"
						end
					end
				end
			end
		end
	end
end

define rtk-task
	set $rtk_task = (T_TaskDescriptor *)$arg0
	if $rtk_task == 0
		printf "RTK task: NULL\n"
	else
		set $rtk_status = (unsigned int)$rtk_task->TaskStatus.AsByte
		set $rtk_wait_object = (unsigned int)$rtk_task->ObjectToWait.DW
		set $rtk_wait_param = (unsigned int)$rtk_task->Param.DW_Param
		set $rtk_wait_caller = (unsigned int)$rtk_task->WaitCallerAddress
		set $rtk_priority = (unsigned int)$rtk_task->TaskPriority

		printf "\nRTK task 0x%08x", (unsigned int)$rtk_task
		if $rtk_task == CurrentTaskPtr
			printf "  CURRENT"
		end
		printf "\n"

		printf "  Label              \""
		rtk-print-label64 $rtk_task->Label
		printf "\"\n"

		printf "  Priority           %u (", $rtk_priority
		rtk-priority-name $rtk_priority
		printf ")\n"

		printf "  Status             0x%02x  ", $rtk_status
		rtk-wait-name $rtk_status
		printf "\n"

		printf "  Wait object        0x%08x\n", $rtk_wait_object
		printf "  Wait object symbol "
		info symbol $rtk_wait_object
		printf "  Wait param         0x%08x\n", $rtk_wait_param
		printf "  Wait caller        0x%08x", $rtk_wait_caller
		if $rtk_wait_caller != 0
			printf "  "
			info symbol $rtk_wait_caller
			info line *$rtk_wait_caller
		else
			printf "\n"
		end

		printf "  PSP                0x%08x\n", (unsigned int)$rtk_task->PSP
		printf "  Saved PC           0x%08x  ", (unsigned int)$rtk_task->PSP->PC
		info symbol $rtk_task->PSP->PC
		info line *$rtk_task->PSP->PC
		printf "  Saved LR           0x%08x  ", (unsigned int)$rtk_task->PSP->LR
		info symbol $rtk_task->PSP->LR
		info line *$rtk_task->PSP->LR
		printf "  Next               0x%08x\n\n", (unsigned int)$rtk_task->Next
	end
end

define rtk-task-line
	set $rtk_task = (T_TaskDescriptor *)$arg0
	if $rtk_task == 0
		printf "  NULL\n"
	else
		set $rtk_status = (unsigned int)$rtk_task->TaskStatus.AsByte
		set $rtk_priority = (unsigned int)$rtk_task->TaskPriority
		printf "  "
		if $rtk_task == CurrentTaskPtr
			printf "* "
		else
			printf "  "
		end
		printf "0x%08x  \"", (unsigned int)$rtk_task
		rtk-print-label64 $rtk_task->Label
		printf "\"  "
		rtk-priority-name $rtk_priority
		printf "  "
		rtk-wait-name $rtk_status
		printf "\n"
	end
end

define rtk-list
	set $rtk_list = (T_TaskDescriptor *)$arg0
	if $rtk_list == 0
		printf "  <empty>\n"
	else
		set $rtk_first = $rtk_list
		set $rtk_task = $rtk_list
		set $rtk_count = 0
		while $rtk_count < 64
			rtk-task-line $rtk_task
			set $rtk_task = $rtk_task->Next
			set $rtk_count = $rtk_count + 1
			if $rtk_task == $rtk_first
				set $rtk_count = 64
			end
		end
		if $rtk_task != $rtk_first
			printf "  <stopped after 64 nodes: possible corrupted circular list>\n"
		end
	end
end

define rtk-tasks
	printf "\nRTK task lists\n"
	printf "\nCritical:\n"
	rtk-list CriticalProcList
	printf "\nHigh:\n"
	rtk-list HiPriProcList
	printf "\nMedium:\n"
	rtk-list MediumPriProcList
	printf "\nLow:\n"
	rtk-list LowPriProcList
	printf "\nBackground:\n"
	rtk-list BkGroundProcList
	printf "\nIdle:\n"
	if IdleTaskDescriptor == 0
		printf "  <not created>\n"
	else
		rtk-task-line IdleTaskDescriptor
	end
	printf "\n"
end

define rtk-find-label-in-list
	set $rtk_list = (T_TaskDescriptor *)$arg0
	set $rtk_target_label = (unsigned long long)$arg1
	if $rtk_list != 0
		set $rtk_first = $rtk_list
		set $rtk_task = $rtk_list
		set $rtk_count = 0
		while $rtk_count < 64
			if ((unsigned long long)$rtk_task->Label) == $rtk_target_label
				set $rtk_last_match = $rtk_task
				set $rtk_match_count = $rtk_match_count + 1
				rtk-task-line $rtk_task
			end
			set $rtk_task = $rtk_task->Next
			set $rtk_count = $rtk_count + 1
			if $rtk_task == $rtk_first
				set $rtk_count = 64
			end
		end
	end
end

define rtk-task-label
	set $rtk_target_label = (unsigned long long)$arg0
	set $rtk_match_count = 0
	set $rtk_last_match = 0

	printf "\nRTK tasks matching packed label 0x%016llx\n", $rtk_target_label
	rtk-find-label-in-list CriticalProcList $rtk_target_label
	rtk-find-label-in-list HiPriProcList $rtk_target_label
	rtk-find-label-in-list MediumPriProcList $rtk_target_label
	rtk-find-label-in-list LowPriProcList $rtk_target_label
	rtk-find-label-in-list BkGroundProcList $rtk_target_label
	if IdleTaskDescriptor != 0
		if ((unsigned long long)IdleTaskDescriptor->Label) == $rtk_target_label
			set $rtk_last_match = IdleTaskDescriptor
			set $rtk_match_count = $rtk_match_count + 1
			rtk-task-line IdleTaskDescriptor
		end
	end

	if $rtk_match_count == 0
		printf "  <no matching task>\n\n"
	else
		if $rtk_match_count == 1
			rtk-task $rtk_last_match
		else
			printf "\nMultiple matching tasks. Use rtk-task <address> on the selected row.\n\n"
		end
	end
end

define rtk-current
	if CurrentTaskPtr == 0
		printf "RTK current task is NULL. Scheduler may not be running yet.\n"
	else
		rtk-task CurrentTaskPtr
	end
end

document rtk-current
Print the current RTK task descriptor decoded for human inspection.
end

document rtk-task
Print one RTK task descriptor. Usage: rtk-task <task_ptr>
end

document rtk-tasks
Print all RTK task lists with a compact per-task status line.
end

document rtk-task-label
Find tasks by packed 64-bit RTK label. Usage: rtk-task-label <packed_label>
Example: rtk-task-label MainTaskHND->Label
end

document rtk-wait-name
Decode an RTK task wait status byte. Usage: rtk-wait-name <status>
end

printf "RTK console commands loaded. Use: rtk-current, rtk-task <ptr>, rtk-tasks\n"
