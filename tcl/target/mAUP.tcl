# ====================================================================================
echo "    reb                         (Setup for an EJTAGBOOT indicated reset via Sharta-A S8.)"
# ====================================================================================
proc reb {} {
	halt
    mww 0xbfc00000 0x1000ffff ;# branch-self
    mww 0xbfc00004 0x00000000 ;# nop
    irscan mAUP.cpu 0x0c ;# EJTAGBOOT
    echo "Press and Hold SW2 then release SW6 (will result in system reset w/ mAUC EJTAGBOOT.)"
}

# ====================================================================================
echo "    rnb                         (Setup for a NORNALBOOT indicated reset via Sharta-A S8.)"
# ====================================================================================
proc rnb {} {
	irscan mAUP.cpu 0x0d ;# NORMALBOOT
    echo "Press and release SW8 (will result in system reset w/ mAUC NORMALBOOT.)"
}

# ====================================================================================
echo "    loaduP"
# ====================================================================================
proc loaduP {} {
	load_image Native_Image;#load image
	mips32 cp0 depc 0x90000000;# Set DEPC
	reg pc 0x90000000;# Set PC
    reg status 0x10400104;# BEV == ERL == 1
	step
	reg pc 0x90000011;# Set PC
}

# ====================================================================================
echo "    fastq <mode>          (mode 0/1 disables/enables 'fast queued mode'.)"
# ====================================================================================
# Workaround: "fast queued" mode making faulty assumptions (but need it to expose use of FASTDATA.)
proc fastq {mode} {
    if {$mode} {
        # FixMe: lower once error detection, propagation, and recovery are improved.
        # mips32 scan_delay 50000 ;# 50 uS "wait and assume next access is pending." 
        mips32 scan_delay 200000 ;# 200 uS "wait and assume next access is pending." 
    } else {
        # FixMe: Use of EJTAG FASTDATA support should independent of queued mode.)
        mips32 scan_delay 2000000 ;# Legacy mode. Full/correct dmseg access handshake.
    }
}

# ====================================================================================
echo "    myload                      (Example of FASTDATA load, verify, invalidate.)"
# ====================================================================================
proc myload {vaddr} {

    # FASTDATA download currently requires enable of "fast queued" mode.
    # The current implementation of this functionality has assumptions which are not valid.
    # The download speeds here will be graetly improved with OpenOCD source fixes...

    fastq 1

    echo "loading..." ; update
    load_image bin64kle $vaddr bin

    echo "verifying..." ; update
    verify_image bin64kle $vaddr bin

    fastq 0
    # invalidate ;# Uncomment if download includes "code".
    # reg pc $vaddr ;# Uncomment if "entry point" at first load address.
}

# ====================================================================================
echo "    invalidate ?nowb?           (Invalidate caches.)"
# ====================================================================================
# Workaround: OpenOCD does not currently have command to invalidate all caches.
proc invalidate {args} {

    # This TEMPORARY WORKAROUND has the following issues:
    # FixMe: don't clobber gpr (at and v0)
    # FixMe: don't clobber cp0 (TagLo and "pc")
    # FixMe: detect cache sizes including unified L2$
    # FixMe: Broken core was in usermode when halted.

    TagLo 0
    reg at 0x80000000 ;# Start
    reg v0 0x80002000 ;# 8 kbytes

    # Index Store Tag (010) I$ (00) == cacheop 8
    mww 0xa0000000  0xBC280000 ;#  cache     0x8,0(at)
    if { [lindex $args 0] == "nowb" } {
        # (option to not writeback dirty lines is useful when there is no bootcode to available to init caches.)
        echo "    Discarding dirty D$ lines..."
        # Index Store Tag (010) D$ (01) == cacheop 9
        mww 0xa0000004  0xBC290000 ;#  cache     0x9,0(at)
    } else {
        # Index Writeback invalidate (000) D$ (01) == cacheop 1
        mww 0xa0000004  0xBC210000 ;#  cache     0x1,0(at)
    }
    mww 0xa0000008  0x1422FFFD ;#  bne       at,v0,0x80000000
    mww 0xa000000C  0x24210010 ;#  addiu     at,at,16
    mww 0xa0000010  0x7000003F ;#  sdbbp

    reg pc 0xa0000000
    resume

    halt 2000 ;# Wait up to 2 seconds to halt
}

# ====================================================================================
echo "    s8rr                        (Recover from pressing Shastra-A switch S8.)"
# ====================================================================================
# Probe interface signal RST* does not reset a Shastra-A memory bus resulting in response skew.
# Workaround: Press Shastra S8 (reset) button and then recover debug comms w/ mAUP.
proc s8rr {} {

    # S8 reset "recovery"
    echo "    Waiting a few seconds for mAUC to power up mAUP..."
    after 2000

    echo "    Reseting targer TAP via TRST..."
#    jtag_reset 1 0 ;# Make sure target TAP is in sync.

    mAUP.cpu curstate
    echo "    Halting target..."
    halt

    # FixMe: Delay slot is not set by mAUC firmware
    # mww 0xbfc00000 0x1000ffff ;# branch-self
    mww 0xbfc00004 0x00000000 ;# nop
    mdw 0xbfc00000
    mdw 0xbfc00004

    reg status 0x00400004 ;# BEV == ERL == 1

    # FixMe: Boot exception vectors not covered by mAUC firmware so cover with debugger.
    # bp 0xbfc00200 4 hw
    # bp 0xbfc00300 4 hw
    # bp 0xbfc00380 4 hw

    # invalidate nowb ;# No boot code or OpenOCD command to init caches so script it here.

    # Wait for first source modificatios to clean up memory issues exposed by enabling caches.
    # echo "    Marking kseg0 as cacheable..."
    # FixMe: Unable to find workaround to cached memory access so don't enable until OpenOCD source fixed.
    # Config 0x80200483 ;# FixMe: let boot code make kseg0 write-back/write-allocate
    reg pc 0xbfc00000 ;# FixMe: Forcing pc back to reset vector.
}

# Used for manually sizing memories including detecting block aliasing...
proc pokepeek {addr offset count} {
    
    for {set i 0} {$i < $count} {incr i} {
        set myaddr [expr $addr + ($offset * $i)]
        mww $myaddr $myaddr
        update
    }
    for {set i 0} {$i < $count} {incr i} {
        set myaddr [expr $addr + ($offset * $i)]
        mdw $myaddr
        update
    }
}

# ====================================================================================
# echo "leaving 'mAUP.tcl'"
# ====================================================================================
return ""
