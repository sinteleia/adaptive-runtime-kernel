import gdb


def _u32(value):
    return int(value) & 0xFFFFFFFF


def _field(snapshot, name):
    return _u32(snapshot[name])


def _symbol(addr):
    if addr == 0:
        return "n/a"
    try:
        text = gdb.execute("info symbol 0x%08x" % addr, to_string=True).strip()
    except gdb.error:
        text = "no symbol"
    try:
        sal = gdb.find_pc_line(addr)
        if sal.symtab and sal.line:
            return "%s (%s:%d)" % (text, sal.symtab.filename, sal.line)
    except Exception:
        pass
    return text


def _flag(value, mask):
    return (value & mask) != 0


def _decode_cfsr(cfsr, mmfar, bfar):
    lines = []
    if _flag(cfsr, 1 << 0):
        lines.append("MemManage: instruction access violation")
    if _flag(cfsr, 1 << 1):
        lines.append("MemManage: data access violation")
    if _flag(cfsr, 1 << 3):
        lines.append("MemManage: unstacking error during exception return")
    if _flag(cfsr, 1 << 4):
        lines.append("MemManage: stacking error during exception entry")
    if _flag(cfsr, 1 << 5):
        lines.append("MemManage: lazy FPU state preservation error")
    if _flag(cfsr, 1 << 7):
        lines.append("MemManage fault address valid: 0x%08x" % mmfar)
    if _flag(cfsr, 1 << 8):
        lines.append("BusFault: instruction bus error")
    if _flag(cfsr, 1 << 9):
        lines.append("BusFault: precise data access error")
    if _flag(cfsr, 1 << 10):
        lines.append("BusFault: imprecise data access error; saved PC may be after the failing store")
    if _flag(cfsr, 1 << 11):
        lines.append("BusFault: unstacking error during exception return")
    if _flag(cfsr, 1 << 12):
        lines.append("BusFault: stacking error during exception entry")
    if _flag(cfsr, 1 << 13):
        lines.append("BusFault: lazy FPU state preservation error")
    if _flag(cfsr, 1 << 15):
        lines.append("BusFault address valid: 0x%08x" % bfar)
    if _flag(cfsr, 1 << 16):
        lines.append("UsageFault: undefined instruction")
    if _flag(cfsr, 1 << 17):
        lines.append("UsageFault: invalid EPSR state")
    if _flag(cfsr, 1 << 18):
        lines.append("UsageFault: invalid exception return")
    if _flag(cfsr, 1 << 19):
        lines.append("UsageFault: coprocessor or FPU access not enabled")
    if _flag(cfsr, 1 << 24):
        lines.append("UsageFault: unaligned memory access")
    if _flag(cfsr, 1 << 25):
        lines.append("UsageFault: division by zero")
    return lines or ["No CFSR cause bit is set"]


def _decode_hfsr(hfsr):
    lines = []
    if _flag(hfsr, 1 << 1):
        lines.append("HardFault: vector table read fault")
    if _flag(hfsr, 1 << 30):
        lines.append("HardFault: escalated configurable fault")
    if _flag(hfsr, 1 << 31):
        lines.append("HardFault: debug event")
    return lines or ["No HFSR cause bit is set"]


class ArkFault(gdb.Command):
    def __init__(self):
        super(ArkFault, self).__init__("ark-fault", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        try:
            snapshot = gdb.parse_and_eval("ErrorTrapSnapshot")
        except gdb.error as exc:
            gdb.write("ErrorTrapSnapshot is not available: %s\n" % exc)
            return

        magic = _field(snapshot, "Magic")
        if magic != 0x45545250:
            gdb.write("ARK fault snapshot is not valid: Magic=0x%08x\n" % magic)
            return

        cfsr = _u32(gdb.parse_and_eval("_CFSR.AsDW"))
        hfsr = _u32(gdb.parse_and_eval("_HFSR.AsDW"))
        dfsr = _u32(gdb.parse_and_eval("_DFSR.AsDW"))
        afsr = _u32(gdb.parse_and_eval("_AFSR"))
        mmfar = _u32(gdb.parse_and_eval("_MMFAR"))
        bfar = _u32(gdb.parse_and_eval("_BFAR"))
        stacked_pc = _u32(gdb.parse_and_eval("stacked_pc"))
        stacked_lr = _u32(gdb.parse_and_eval("stacked_lr"))

        context_exception = _field(snapshot, "FaultContextException")
        is_task = _field(snapshot, "FaultContextIsTask") != 0
        is_isr = _field(snapshot, "FaultContextIsISR") != 0

        gdb.write("\nARK fault snapshot, version %u\n" % _field(snapshot, "Version"))
        if is_task:
            gdb.write("Context: RTK task\n")
        elif is_isr:
            gdb.write("Context: ISR/exception number %u\n" % context_exception)
        else:
            gdb.write("Context: Thread mode outside a recognized RTK task\n")

        gdb.write("\nFaulted code:\n")
        gdb.write("  PC  0x%08x  %s\n" % (stacked_pc, _symbol(stacked_pc)))
        gdb.write("  LR  0x%08x  %s\n" % (stacked_lr, _symbol(stacked_lr)))

        gdb.write("\nTask snapshot:\n")
        gdb.write("  CurrentTaskPtr     0x%08x\n" % _field(snapshot, "CurrentTaskPtr"))
        gdb.write("  CurrentTaskSavedPC 0x%08x  %s\n" %
                  (_field(snapshot, "CurrentTaskSavedPC"), _symbol(_field(snapshot, "CurrentTaskSavedPC"))))
        gdb.write("  CurrentTaskSavedLR 0x%08x  %s\n" %
                  (_field(snapshot, "CurrentTaskSavedLR"), _symbol(_field(snapshot, "CurrentTaskSavedLR"))))
        gdb.write("  Priority           0x%02x\n" % _field(snapshot, "CurrentTaskPriority"))
        gdb.write("  Status             0x%02x\n" % _field(snapshot, "CurrentTaskStatus"))
        gdb.write("  Label raw          0x%08x\n" % _field(snapshot, "CurrentTaskLabel"))
        gdb.write("  Wait object        0x%08x\n" % _field(snapshot, "CurrentTaskWaitObject"))
        gdb.write("  Wait param         0x%08x\n" % _field(snapshot, "CurrentTaskWaitParam"))
        gdb.write("  Wait caller        0x%08x  %s\n" %
                  (_field(snapshot, "CurrentTaskWaitCallerAddress"),
                   _symbol(_field(snapshot, "CurrentTaskWaitCallerAddress"))))

        gdb.write("\nFault cause:\n")
        for line in _decode_hfsr(hfsr):
            gdb.write("  %s\n" % line)
        for line in _decode_cfsr(cfsr, mmfar, bfar):
            gdb.write("  %s\n" % line)

        gdb.write("\nRaw registers:\n")
        gdb.write("  CFSR=0x%08x HFSR=0x%08x DFSR=0x%08x AFSR=0x%08x\n" %
                  (cfsr, hfsr, dfsr, afsr))
        gdb.write("  MMFAR=0x%08x BFAR=0x%08x\n" % (mmfar, bfar))
        gdb.write("  EXC_RETURN=0x%08x CONTROL=0x%08x IPSR=0x%08x\n" %
                  (_field(snapshot, "ExcReturn"), _field(snapshot, "CONTROL"), _field(snapshot, "IPSR")))
        gdb.write("  PSP=0x%08x MSP=0x%08x BASEPRI=0x%08x PRIMASK=0x%08x FAULTMASK=0x%08x\n\n" %
                  (_field(snapshot, "PSP"), _field(snapshot, "MSP"),
                   _field(snapshot, "BASEPRI"), _field(snapshot, "PRIMASK"),
                   _field(snapshot, "FAULTMASK")))


ArkFault()
gdb.write("ARK fault command loaded. Use: ark-fault\n")
