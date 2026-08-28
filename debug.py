#!/usr/bin/env python3
"""
SweRV EH2 RISC-V Debug Module Driver
=====================================

FT232H
   |
   +-- TCK
   +-- TMS
   +-- TDI
   +-- TDO
   |
   v
SweRV EH2 JTAG DTM
   |
   v
RISC-V Debug Module
   |
   +-- DMI
   |
   +-- Access Register Abstract Command
   |       |
   |       +-- GPR
   |       +-- DPC
   |       +-- CSR
   |
   +-- Access Memory Abstract Command
   |       |
   |       +-- ICCM
   |       +-- DCCM
   |       +-- PIC
   |
   +-- System Bus Access
           |
           +-- SoC / external memory

重要：
--------
SweRV EH2 的 ICCM / DCCM 不應使用 SBA 直接存取。

ICCM / DCCM：
    Access Memory Abstract Command
        data0 = data
        data1 = address

SoC memory：
    System Bus Access
        SBADDRESS0
        SBDATA0

因此本程式將兩條路徑明確分開。
"""

from __future__ import annotations

import time

from pyftdi.jtag import JtagEngine
from pyftdi.bits import BitSequence


# ============================================================================
# JTAG / DTM
# ============================================================================

JTAG_IR_LENGTH = 5

JTAG_IR_IDCODE = 0x01
JTAG_IR_DTMCS  = 0x10
JTAG_IR_DMI    = 0x11

DMI_LENGTH = 41


# ============================================================================
# DMI operation
# ============================================================================

DMI_NOP   = 0
DMI_READ  = 1
DMI_WRITE = 2


# ============================================================================
# Debug Module Registers
# ============================================================================

DM_DATA0       = 0x04
DM_DATA1       = 0x05

DM_DMCONTROL   = 0x10
DM_DMSTATUS    = 0x11
DM_HARTINFO    = 0x12

DM_ABSTRACTCS  = 0x16
DM_COMMAND     = 0x17
DM_ABSTRACTAUTO = 0x18

DM_PROGBUF0    = 0x20

# System Bus Access
DM_SBCS        = 0x38
DM_SBADDRESS0  = 0x39
DM_SBDATA0     = 0x3C


# ============================================================================
# DMCONTROL
# ============================================================================

DMCONTROL_DMACTIVE  = 1 << 0
DMCONTROL_NDMRESET  = 1 << 1

DMCONTROL_RESUMEREQ = 1 << 30
DMCONTROL_HALTREQ   = 1 << 31


# ============================================================================
# DMSTATUS
# ============================================================================

DMSTATUS_ANYRUNNING   = 1 << 6
DMSTATUS_ALLRUNNING   = 1 << 7

DMSTATUS_ANYHALTED    = 1 << 8
DMSTATUS_ALLHALTED    = 1 << 9

DMSTATUS_ANYRESUMEACK = 1 << 16
DMSTATUS_ALLRESUMEACK = 1 << 17


# ============================================================================
# Abstract Command
# ============================================================================

# cmdtype = 0
ABSTRACT_CMD_ACCESS_REGISTER = 0x00

# cmdtype = 2
ABSTRACT_CMD_ACCESS_MEMORY = 0x02 << 24


# ---------------------------------------------------------------------------
# Access Register Command
# ---------------------------------------------------------------------------

# aarsize = 2 -> 32-bit
ABSTRACT_AARSIZE_32 = 2 << 20

# bit 17
ABSTRACT_TRANSFER = 1 << 17

# bit 16
ABSTRACT_WRITE = 1 << 16


# ---------------------------------------------------------------------------
# Access Memory Command
# ---------------------------------------------------------------------------

# aamsize = 0 -> 8-bit
# aamsize = 1 -> 16-bit
# aamsize = 2 -> 32-bit
# aamsize = 3 -> 64-bit
ABSTRACT_AAMSIZE_8  = 0 << 20
ABSTRACT_AAMSIZE_16 = 1 << 20
ABSTRACT_AAMSIZE_32 = 2 << 20

# bit 23
ABSTRACT_AAMVIRTUAL = 1 << 23

# bit 19
ABSTRACT_POSTINCREMENT = 1 << 19

# bit 18
ABSTRACT_POSTEXEC = 1 << 18

# bit 16
ABSTRACT_MEM_WRITE = 1 << 16


# ============================================================================
# RISC-V Register Numbers
# ============================================================================

# GPR:
#
# x0  = 0x1000
# x1  = 0x1001
# ...
# x31 = 0x101f

REGNO_DPC = 0x7B1
REGNO_MSTATUS = 0x7B2
REGNO_MISA = 0x7B3
REGNO_MIE = 0x7B4
REGNO_MTVEC = 0x7B5
REGNO_CYCH = 0xB80
REGNO_CYCL = 0xB00


# ============================================================================
# System Bus Access
# ============================================================================

# sbaccess:
#
# 0 = 8 bit
# 1 = 16 bit
# 2 = 32 bit
# 3 = 64 bit

SBCS_SBACCESS_8  = 0 << 17
SBCS_SBACCESS_16 = 1 << 17
SBCS_SBACCESS_32 = 2 << 17
SBCS_SBACCESS_64 = 3 << 17

SBCS_SBREADONADDR     = 1 << 15
SBCS_SBAUTOINCREMENT  = 1 << 16

SBCS_SBBUSY           = 1 << 21
SBCS_SBBUSYERROR      = 1 << 22

SBCS_SBERROR_MASK     = 0x7 << 12


# ============================================================================
# Exceptions
# ============================================================================

class SwervDmiError(RuntimeError):
    pass


class DmiError(SwervDmiError):
    pass


class AbstractCommandError(SwervDmiError):
    pass


class SystemBusError(SwervDmiError):
    pass


# ============================================================================
# SweRV Debugger
# ============================================================================

class SweRV:

    def __init__(
        self,
        url: str = "ftdi://ftdi:232h/1",
        frequency: float = 1_000_000,
        verbose: bool = False,
    ):

        self.url = url
        self.frequency = frequency
        self.verbose = verbose

        self.jtag = JtagEngine(
            trst=False,
            frequency=frequency,
        )

    # ========================================================================
    # Connection
    # ========================================================================

    def connect(self):

        if self.verbose:
            print(f"[+] FTDI: {self.url}")
            print(f"[+] JTAG frequency: {self.frequency:.0f} Hz")

        self.jtag.configure(self.url)

        self.reset_tap()

        self.jtag.sync()

    def close(self):

        try:
            self.jtag.close()
        except Exception:
            pass

    # ========================================================================
    # JTAG
    # ========================================================================

    def reset_tap(self):

        if self.verbose:
            print("[JTAG] TAP reset")

        self.jtag.reset()

    def ir_scan(self, instruction: int):

        if not 0 <= instruction < (1 << JTAG_IR_LENGTH):
            raise ValueError(
                "JTAG instruction does not fit in 5 bits"
            )

        bs = BitSequence(
            instruction,
            length=JTAG_IR_LENGTH,
        )

        self.jtag.write_ir(bs)
        self.jtag.go_idle()

    def dr_scan(self, value: int, length: int) -> int:

        if length <= 0:
            raise ValueError("Invalid DR length")

        bs = BitSequence(
            value,
            length=length,
        )

        self.jtag.change_state("shift_dr")

        rx = self.jtag.shift_and_update_register(bs)

        self.jtag.go_idle()

        return int(rx)

    # ========================================================================
    # IDCODE
    # ========================================================================

    def idcode(self) -> int:

        self.ir_scan(JTAG_IR_IDCODE)

        value = self.dr_scan(
            0,
            32,
        )

        if self.verbose:
            print(
                f"[JTAG] IDCODE = 0x{value:08x}"
            )

        return value

    # ========================================================================
    # DTMCS
    # ========================================================================

    def dtmcs(self) -> int:

        self.ir_scan(JTAG_IR_DTMCS)

        value = self.dr_scan(
            0,
            32,
        )

        if self.verbose:

            version = value & 0xf
            abits = (value >> 4) & 0x3f
            dmistat = (value >> 10) & 0x3
            idle = (value >> 12) & 0x7

            print(
                "[DTMCS] "
                f"raw=0x{value:08x} "
                f"version={version} "
                f"abits={abits} "
                f"dmistat={dmistat} "
                f"idle={idle}"
            )

        return value

    # ========================================================================
    # DMI
    # ========================================================================

    @staticmethod
    def make_dmi_request(
        address: int,
        data: int,
        operation: int,
    ) -> int:

        if not 0 <= address < (1 << 7):
            raise ValueError(
                "DMI address must fit in 7 bits"
            )

        if not 0 <= data < (1 << 32):
            raise ValueError(
                "DMI data must fit in 32 bits"
            )

        return (
            (address << 34)
            | ((data & 0xffffffff) << 2)
            | operation
        )

    @staticmethod
    def decode_dmi_response(value: int):

        operation = value & 0x3

        data = (
            (value >> 2)
            & 0xffffffff
        )

        address = (
            (value >> 34)
            & 0x7f
        )

        return address, data, operation

    def dmi_raw(self, request: int) -> int:

        self.ir_scan(JTAG_IR_DMI)

        return self.dr_scan(
            request,
            DMI_LENGTH,
        )

    def dmi_nop(self) -> int:

        request = self.make_dmi_request(
            0,
            0,
            DMI_NOP,
        )

        return self.dmi_raw(request)

    def dmi_read(self, address: int) -> int:

        request = self.make_dmi_request(
            address,
            0,
            DMI_READ,
        )

        # 第一個 scan：
        #
        # 將 READ request 送進 DTM。
        self.dmi_raw(request)

        # 第二個 scan：
        #
        # 用 NOP 把前一次 READ 的結果取回。
        response = self.dmi_nop()

        raddr, data, operation = \
            self.decode_dmi_response(response)

        if operation == 3:
            raise DmiError(
                f"DMI error: "
                f"addr=0x{raddr:02x}"
            )

        if self.verbose:
            print(
                f"[DMI] READ  "
                f"addr=0x{address:02x} "
                f"data=0x{data:08x}"
            )

        return data

    def dmi_write(
        self,
        address: int,
        data: int,
    ):

        request = self.make_dmi_request(
            address,
            data,
            DMI_WRITE,
        )

        response = self.dmi_raw(request)

        if self.verbose:
            print(
                f"[DMI-WRITE] "
                f"addr=0x{address:02x} "
                f"data=0x{data:08x} "
                f"response=0x{response:011x}"
            )

        # DMI write 同樣需要 flush pipeline。
        self.dmi_nop()

    # ========================================================================
    # Debug Module register access
    # ========================================================================

    def dm_read(self, address: int) -> int:

        return self.dmi_read(address)

    def dm_write(
        self,
        address: int,
        value: int,
    ):

        self.dmi_write(
            address,
            value,
        )

    # ========================================================================
    # DM activation
    # ========================================================================

    def activate(self, timeout: float = 1.0):

        # 先把 DM inactive。
        self.dm_write(
            DM_DMCONTROL,
            0,
        )

        deadline = time.monotonic() + timeout

        while time.monotonic() < deadline:

            value = self.dm_read(
                DM_DMCONTROL
            )

            if value == 0:
                break

            time.sleep(0.001)

        if value != 0:

            raise SwervDmiError(
                "DM failed to deactivate: "
                f"dmcontrol=0x{value:08x}"
            )

        # 再啟動 Debug Module。
        self.dm_write(
            DM_DMCONTROL,
            DMCONTROL_DMACTIVE,
        )

        deadline = time.monotonic() + timeout

        while time.monotonic() < deadline:

            value = self.dm_read(
                DM_DMCONTROL
            )

            if value & DMCONTROL_DMACTIVE:

                if self.verbose:
                    print(
                        "[DM] active: "
                        f"dmcontrol=0x{value:08x}"
                    )

                return

            time.sleep(0.001)

        raise SwervDmiError(
            "Debug Module did not become active: "
            f"dmcontrol=0x{value:08x}"
        )

    # ========================================================================
    # DM status
    # ========================================================================

    def dmstatus(self) -> int:

        return self.dm_read(
            DM_DMSTATUS
        )

    def dmcontrol(self) -> int:

        return self.dm_read(
            DM_DMCONTROL
        )

    # ========================================================================
    # Halt
    # ========================================================================

    def halt(
        self,
        timeout: float = 1.0,
    ):

        self.dm_write(
            DM_DMCONTROL,
            DMCONTROL_DMACTIVE
            | DMCONTROL_HALTREQ,
        )

        deadline = time.monotonic() + timeout

        while time.monotonic() < deadline:

            status = self.dmstatus()

            if status & DMSTATUS_ALLHALTED:

                if self.verbose:
                    print(
                        "[DM] Hart halted "
                        f"(dmstatus=0x{status:08x})"
                    )

                return

            time.sleep(0.001)

        status = self.dmstatus()

        raise SwervDmiError(
            "Timeout waiting for hart halt: "
            f"dmstatus=0x{status:08x}"
        )

    # ========================================================================
    # Resume
    # ========================================================================

    def resume(
        self,
        timeout: float = 1.0,
    ):

        self.dm_write(
            DM_DMCONTROL,
            DMCONTROL_DMACTIVE
            | DMCONTROL_RESUMEREQ,
        )

        deadline = time.monotonic() + timeout

        while time.monotonic() < deadline:

            status = self.dmstatus()

            if status & DMSTATUS_ALLRESUMEACK:

                if self.verbose:
                    print(
                        "[DM] Resume acknowledged "
                        f"(dmstatus=0x{status:08x}"
                    )

                return

            time.sleep(0.001)

        status = self.dmstatus()

        # 有些 DM 不一定會如預期呈現 RESUMEACK，
        # 但如果 hart 已經 RUNNING，就視為 resume 成功。
        if status & DMSTATUS_ALLRUNNING:

            if self.verbose:
                print(
                    "[DM] Hart running "
                    f"(dmstatus=0x{status:08x})"
                )

            return

        raise SwervDmiError(
            "Timeout waiting for hart resume: "
            f"dmstatus=0x{status:08x}"
        )

    # ========================================================================
    # Abstract Command - common
    # ========================================================================

    def abstractcs(self) -> int:

        return self.dm_read(
            DM_ABSTRACTCS
        )

    def clear_abstract_error(self):

        # cmderr 位於 [10:8]。
        #
        # RISC-V Debug Spec 規定：
        # 寫 1 清除對應 cmderr。
        self.dm_write(
            DM_ABSTRACTCS,
            0x7 << 8,
        )

    def wait_abstract(
        self,
        timeout: float = 1.0,
    ):

        deadline = time.monotonic() + timeout

        while time.monotonic() < deadline:

            acs = self.abstractcs()

            # abstractcs[12]
            busy = bool(
                acs & (1 << 12)
            )

            # abstractcs[10:8]
            cmderr = (
                (acs >> 8)
                & 0x7
            )

            if not busy:

                if cmderr:

                    raise AbstractCommandError(
                        "Abstract command error: "
                        f"cmderr={cmderr}, "
                        f"abstractcs=0x{acs:08x}"
                    )

                return

            time.sleep(0.001)

        raise AbstractCommandError(
            "Timeout waiting for abstract command"
        )

    def require_halted(self):

        status = self.dmstatus()

        if not (
            status & DMSTATUS_ALLHALTED
        ):

            raise SwervDmiError(
                "Hart must be halted for abstract "
                "memory access: "
                f"dmstatus=0x{status:08x}"
            )

    # ========================================================================
    # Access Register Abstract Command
    # ========================================================================

    @staticmethod
    def regno_gpr(index: int) -> int:

        if not 0 <= index <= 31:
            raise ValueError(
                "GPR index must be 0..31"
            )

        return 0x1000 + index

    @staticmethod
    def make_access_register_command(
        regno: int,
        write: bool = False,
    ) -> int:

        command = (
            ABSTRACT_CMD_ACCESS_REGISTER
            | ABSTRACT_AARSIZE_32
            | ABSTRACT_TRANSFER
            | (
                ABSTRACT_WRITE
                if write
                else 0
            )
            | (regno & 0xffff)
        )

        return command

    def read_regno(
        self,
        regno: int,
    ) -> int:

        self.require_halted()

        self.clear_abstract_error()

        command = self.make_access_register_command(
            regno,
            write=False,
        )

        if self.verbose:
            print(
                "[ABSTRACT-REG] READ "
                f"regno=0x{regno:04x} "
                f"command=0x{command:08x}"
            )

        self.dm_write(
            DM_COMMAND,
            command,
        )

        self.wait_abstract()

        value = self.dm_read(
            DM_DATA0
        )

        if self.verbose:
            print(
                "[ABSTRACT-REG] "
                f"DATA0=0x{value:08x}"
            )

        return value

    def write_regno(
        self,
        regno: int,
        value: int,
    ):

        self.require_halted()

        self.clear_abstract_error()

        self.dm_write(
            DM_DATA0,
            value,
        )

        command = self.make_access_register_command(
            regno,
            write=True,
        )

        self.dm_write(
            DM_COMMAND,
            command,
        )

        self.wait_abstract()

    # ========================================================================
    # GPR
    # ========================================================================

    def read_gpr(
        self,
        index: int,
    ) -> int:

        return self.read_regno(
            self.regno_gpr(index)
        )

    def write_gpr(
        self,
        index: int,
        value: int,
    ):

        self.write_regno(
            self.regno_gpr(index),
            value,
        )

    # ========================================================================
    # DPC
    # ========================================================================

    def read_pc(self) -> int:

        return self.read_regno(
            REGNO_DPC
        )

    def write_pc(
        self,
        value: int,
    ):

        self.write_regno(
            REGNO_DPC,
            value,
        )

    # ========================================================================
    # Access Memory Abstract Command
    # ========================================================================

    @staticmethod
    def make_access_memory_command(
        write: bool = False,
        size: int = 32,
    ) -> int:
        """
        建立 Access Memory Abstract Command。

        注意：
        --------------------------
        address 不在 command 裡面。

        address 必須放在 DATA1。

        DATA0：
            read  -> command 完成後回傳 memory data
            write -> 要寫入 memory 的 data

        DATA1：
            memory address

        32-bit command：

            [31:24] cmdtype = 2
            [23]    aamvirtual
            [22:20] aamsize
            [19]    postincrement
            [18]    postexec
            [17:16] 保留 / write control
        """

        if size == 8:
            aamsize = ABSTRACT_AAMSIZE_8

        elif size == 16:
            aamsize = ABSTRACT_AAMSIZE_16

        elif size == 32:
            aamsize = ABSTRACT_AAMSIZE_32

        else:
            raise ValueError(
                "Only 8/16/32-bit memory access "
                "is supported"
            )

        command = (
            ABSTRACT_CMD_ACCESS_MEMORY
            | aamsize
        )

        if write:
            command |= ABSTRACT_MEM_WRITE

        return command

    def read_mem_abstract(
        self,
        address: int,
    ) -> int:
        """
        使用 Access Memory Abstract Command
        讀取 ICCM / DCCM / PIC / core internal memory。

        對 EH2 而言，這才是讀取 ICCM/DCCM
        的正確 debug path。
        """

        self.require_halted()

        self.clear_abstract_error()

        # ------------------------------------------------------------
        # DATA1 = memory address
        #
        # 這一點非常重要：
        #
        # command 本身不包含 address。
        # address 是透過 data1 傳入。
        # ------------------------------------------------------------

        self.dm_write(
            DM_DATA1,
            address,
        )

        # ------------------------------------------------------------
        # 建立：
        #
        # cmdtype = 2
        # aamsize = 32-bit
        # write   = 0
        #
        # 所以 command = 0x02200000
        # ------------------------------------------------------------

        command = self.make_access_memory_command(
            write=False,
            size=32,
        )

        if self.verbose:

            print(
                "[ABSTRACT-MEM] READ REQUEST"
            )

            print(
                f"    DATA1(address) = "
                f"0x{address:08x}"
            )

            print(
                f"    COMMAND        = "
                f"0x{command:08x}"
            )

        # ------------------------------------------------------------
        # 寫入 command 後，EH2 Debug Module 執行 memory access。
        # ------------------------------------------------------------

        self.dm_write(
            DM_COMMAND,
            command,
        )

        # 等待 abstract command 完成。
        self.wait_abstract()

        # ------------------------------------------------------------
        # command 完成後：
        #
        # DATA0 = memory data
        # ------------------------------------------------------------

        value = self.dm_read(
            DM_DATA0
        )

        if self.verbose:

            print(
                "[ABSTRACT-MEM] READ RESULT"
            )

            print(
                f"    address = 0x{address:08x}"
            )

            print(
                f"    data    = 0x{value:08x}"
            )

        return value

    def write_mem_abstract(
        self,
        address: int,
        value: int,
    ):
        """
        使用 Access Memory Abstract Command
        寫入 ICCM / DCCM / core internal memory。
        """

        self.require_halted()

        self.clear_abstract_error()

        # ------------------------------------------------------------
        # DATA0 = 要寫入 memory 的資料
        # DATA1 = memory address
        # ------------------------------------------------------------

        self.dm_write(
            DM_DATA0,
            value,
        )

        self.dm_write(
            DM_DATA1,
            address,
        )

        command = self.make_access_memory_command(
            write=True,
            size=32,
        )

        if self.verbose:

            print(
                "[ABSTRACT-MEM] WRITE REQUEST"
            )

            print(
                f"    DATA0(data)    = "
                f"0x{value:08x}"
            )

            print(
                f"    DATA1(address) = "
                f"0x{address:08x}"
            )

            print(
                f"    COMMAND        = "
                f"0x{command:08x}"
            )

        self.dm_write(
            DM_COMMAND,
            command,
        )

        self.wait_abstract()

        if self.verbose:

            print(
                "[ABSTRACT-MEM] WRITE DONE"
            )

    # ========================================================================
    # Instruction read
    # ========================================================================

    def read_instruction(
        self,
        pc: int,
    ) -> int:
        """
        讀取 PC 所指向的 32-bit instruction。

        注意：
        EH2 是 RV32IMAC，因此 instruction 可能是：

            16-bit compressed instruction
            或
            32-bit instruction

        這裡先以 32-bit memory access 讀出完整 word。
        """

        instruction_word = self.read_mem_abstract(
            pc
        )

        if self.verbose:

            print(
                f"[INST] PC=0x{pc:08x} "
                f"WORD=0x{instruction_word:08x}"
            )

        return instruction_word

    # ========================================================================
    # System Bus Access
    # ========================================================================

    def sbcs(self) -> int:

        return self.dm_read(
            DM_SBCS
        )

    def sb_wait(
        self,
        timeout: float = 1.0,
    ):

        deadline = time.monotonic() + timeout

        while time.monotonic() < deadline:

            sbcs = self.sbcs()

            if sbcs & SBCS_SBBUSYERROR:

                raise SystemBusError(
                    f"SBA busy error: "
                    f"sbcs=0x{sbcs:08x}"
                )

            if sbcs & SBCS_SBERROR_MASK:

                sberror = (
                    (sbcs >> 12)
                    & 0x7
                )

                raise SystemBusError(
                    f"SBA error={sberror}, "
                    f"sbcs=0x{sbcs:08x}"
                )

            if not (
                sbcs & SBCS_SBBUSY
            ):

                return

            time.sleep(0.001)

        raise SystemBusError(
            "Timeout waiting for system bus"
        )

    def configure_sba_32bit(self):

        # SBA 32-bit access。
        #
        # 注意：
        # 這條路徑是給 SoC memory，
        # 不是給 EH2 ICCM/DCCM。

        value = (
            SBCS_SBACCESS_32
            | SBCS_SBREADONADDR
        )

        self.dm_write(
            DM_SBCS,
            value,
        )

    def read_mem32_sba(
        self,
        address: int,
    ) -> int:

        self.configure_sba_32bit()

        self.dm_write(
            DM_SBADDRESS0,
            address,
        )

        self.sb_wait()

        value = self.dm_read(
            DM_SBDATA0
        )

        self.sb_wait()

        if self.verbose:

            print(
                f"[SBA-MEM] READ "
                f"0x{address:08x} = "
                f"0x{value:08x}"
            )

        return value

    def write_mem32_sba(
        self,
        address: int,
        value: int,
    ):

        self.configure_sba_32bit()

        self.dm_write(
            DM_SBADDRESS0,
            address,
        )

        self.sb_wait()

        self.dm_write(
            DM_SBDATA0,
            value,
        )

        self.sb_wait()

        if self.verbose:

            print(
                f"[SBA-MEM] WRITE "
                f"0x{address:08x} <- "
                f"0x{value:08x}"
            )
    # ========================================================================
    # System Function
    # ========================================================================
    
    def get_freq(self,sec=1) -> float:
        #
        # 取得目前CPU cycle count
        #
        s = self.read_regno(REGNO_CYCH)<<32 | self.read_regno(REGNO_CYCL)
        # 取得目前時間
        t1 = time.time_ns()
        #等待delay 時間
        time.sleep(sec)
        #取得目前cpu cycle count       
        e = self.read_regno(REGNO_CYCH)<<32 | self.read_regno(REGNO_CYCL)
        #取得目前時間
        t2 = time.time_ns()
        # 將cycle count除以時間差，得到頻率 (MHz) ,t2,t1 為了補償regno 讀取約要0.3秒時間 傳回單位為MHz

        return (e-s) /(((t2-t1)/1E+9)*1E+6)
        

# ============================================================================
# Test
# ============================================================================

def main():

    dbg = SweRV(
        url="ftdi://ftdi:232h/1",
        frequency=1_000_000,
        verbose=False,
    )

    try:

        print("========================================")
        print(" SweRV EH2 Debug Test")
        print("========================================")

        # ------------------------------------------------------------
        # JTAG
        # ------------------------------------------------------------

        dbg.connect()

        idcode = dbg.idcode()

        print(f"IDCODE = 0x{idcode:08x}")

        # ------------------------------------------------------------
        # DTM
        # ------------------------------------------------------------

        dtmcs = dbg.dtmcs()

        print(f"DTMCS  = 0x{dtmcs:08x}")

        # ------------------------------------------------------------
        # Debug Module
        # ------------------------------------------------------------

        dbg.activate()

        # ------------------------------------------------------------
        # Halt hart
        # ------------------------------------------------------------

        dbg.halt()

        status = dbg.dmstatus()

        print(
            f"HALTED DMSTATUS = "
            f"0x{status:08x}"
        )
   
        #addr = 0xf0050000
        #data = 0x12345678
        #dbg.write_mem_abstract(addr, data)
       
    
        #data = dbg.read_mem_abstract(addr)
        #print(f"read {addr} = 0x{data:08x}")

        freq = dbg.get_freq(1)

        clk_freq_hz = dbg.read_mem_abstract(0x8000103C)/1E+6

       
       
        print(f"CPU Clock = {freq:.3f} MHz (SoC reg = {clk_freq_hz:.3f} MHz")

        # ------------------------------------------------------------
        # Resume
        # ------------------------------------------------------------
        
        dbg.resume()

        print("CPU resumed")

    finally:

        dbg.close()


if __name__ == "__main__":
    main()