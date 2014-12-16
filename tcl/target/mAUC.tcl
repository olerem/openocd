# ====================================================================================
echo "mAUC.tcl(updated 20140508)"
# ====================================================================================


# ====================================================================================
echo "    reb                         (Setup for an EJTAGBOOT indicated reset via Sharta-A S8.)"
# ====================================================================================
proc reb {} {
    jtag_reset 1 0
    irscan mAUC.cpu 0x0c ;# EJTAGBOOT
    echo "Press and release SW8 (will result in system reset w/ mAUC EJTAGBOOT.)"
}

# ====================================================================================
echo "    rnb                         (Setup for a NORNALBOOT indicated reset via Sharta-A S8.)"
# ====================================================================================
proc rnb {} {
    jtag_reset 1 0
        irscan mAUC.cpu 0x0d ;# NORMALBOOT
    echo "Press and release SW8 (will result in system reset w/ mAUC NORMALBOOT.)"
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

#    fastq 1

    echo "loading..." ; update
    load_image bin64kle $vaddr bin

    echo "verifying..." ; update
    verify_image bin64kle $vaddr bin

#    fastq 0
    # invalidate ;# Uncomment if download includes "code".
    # reg pc $vaddr ;# Uncomment if "entry point" at first load address.
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


# Master CP0 register list.
variable cp0_info
array unset cp0_info
array set cp0_info {
    Index              {  0  0  0 per_vpe }
    MVPControl         {  0  1  0 per_core MT_ASE }
    MVPConf0           {  0  2  0 per_core MT_ASE }
    MVPConf1           {  0  3  0 per_core MT_ASE }
    Random             {  1  0  0 per_vpe }
    VPEControl         {  1  1  0 per_vpe MT_ASE }
    VPEConf0           {  1  2  0 per_vpe MT_ASE }
    VPEConf1           {  1  3  0 per_vpe MT_ASE }
    YQMask             {  1  4  0 per_vpe }
    VPESchedule        {  1  5  0 per_vpe MT_ASE }
    VPEScheFBack       {  1  6  0 per_vpe MT_ASE }
    VPEOpt             {  1  7  0 per_vpe MT_ASE }
    EntryLo0           {  2  0  0 per_vpe }
    TCStatus           {  2  1  0 per_tc MT_ASE }
    TCBind             {  2  2  0 per_tc MT_ASE }
    TCRestart          {  2  3  0 per_tc MT_ASE }
    TCHalt             {  2  4  0 per_tc MT_ASE }
    TCContext          {  2  5  0 per_tc MT_ASE }
    TCSchedule         {  2  6  0 per_tc MT_ASE }
    TCScheFBack        {  2  7  0 per_tc MT_ASE }
    EntryLo1           {  3  0  0 per_vpe }
    TCOpt              {  3  7  0 per_tc MT_ASE }
    Context            {  4  0  0 per_vpe }
    ContextConfig      {  4  1  0 per_vpe SmartMIPS }
    UserLocal          {  4  2 32 per_tc }
    PageMask           {  5  0  0 per_vpe }
    PageGrain          {  5  1  0 per_vpe }
    SegCtl0            {  5  2  0 per_vpe }
    SegCtl1            {  5  3  0 per_vpe }
    SegCtl2            {  5  4  0 per_vpe }
    PWBase             {  5  5  0 per_vpe }
    PWField            {  5  6  0 per_vpe }
    PWSize             {  5  7  0 per_vpe }
    Wired              {  6  0  0 per_vpe }
    SRSConf0           {  6  1  0 per_vpe }
    SRSConf1           {  6  2  0 per_vpe }
    SRSConf2           {  6  3  0 per_vpe }
    SRSConf3           {  6  4  0 per_vpe }
    SRSConf4           {  6  5  0 per_vpe }
    PWCtl              {  6  6  0 per_vpe }
    HWREna             {  7  0 32 per_vpe }
    BadVAddr           {  8  0 32 per_vpe }
    BadInstr           {  8  1  0 per_vpe }
    BadInstrP          {  8  2  0 per_vpe }
    Count              {  9  0 32 per_vpe }
    EntryHi            { 10  0  0 per_tc }
    GuestCtl1          { 10  4  0 per_vpe VZ_MOD }
    GuestCtl2          { 10  5  0 per_vpe VZ_MOD }
    GuestCtl3          { 10  6  0 per_vpe VZ_MOD }
    Compare            { 11  0 32 per_vpe }
    GuestCtl0Ext       { 11  4  0 per_vpe VZ_MOD }
    Status             { 12  0 32 per_tc !DON'T USE! OpenOCD not updating cached copy!}
    IntCtl             { 12  1 32 per_vpe }
    SRSCtl             { 12  2 32 per_vpe }
    SRSMap             { 12  3  0 per_vpe }
    View_IPL           { 12  4 32 per_vpe MCU_ASE }
    SRSMap2            { 12  5  0 per_vpe MCU_ASE }
    GuestCtl0          { 12  6  0 per_vpe VZ_MOD }
    GTOffset           { 12  7  0 per_vpe VZ_MOD }
    Cause              { 13  0 32 per_vpe }
    View_RIPL          { 13  4  0 per_vpe MCU_ASE }
    NestedExc          { 13  5 32 per_vpe }
    EPC                { 14  0 32 per_vpe }
    NestedEPC          { 14  2 32 per_vpe }
    PRId               { 15  0 32 per_vpe }
    EBase              { 15  1 32 per_vpe }
    CDMMBase           { 15  2 32 per_core }
    CMGCRBase          { 15  3  0 per_cps }
    Config             { 16  0 32 per_vpe }
    Config1            { 16  1 32 per_vpe }
    Config2            { 16  2 32 per_vpe }
    Config3            { 16  3 32 per_vpe }
    Config4            { 16  4 32 per_vpe Implementation}
    Config5            { 16  5 32 per_vpe Implementation}
    Config6            { 16  6  0 per_vpe Implementation}
    Config7            { 16  7 32 per_vpe Implementation}
    LLAddr             { 17  0 32 per_tc }
    WatchLo            { 18  0  0 per_vpe }
    WatchLo0           { 18  0  0 per_vpe }
    WatchLo1           { 18  1  0 per_vpe }
    WatchLo2           { 18  2  0 per_vpe }
    WatchLo3           { 18  3  0 per_vpe }
    WatchHi            { 19  0  0 per_vpe }
    WatchHi0           { 19  0  0 per_vpe }
    WatchHi1           { 19  1  0 per_vpe }
    WatchHi2           { 19  2  0 per_vpe }
    WatchHi3           { 19  3  0 per_vpe }
    XContext           { 20  0  0 per_vpe }
    SecurityCtrl       { 22  0  0 per_vpe Implementation }
    SecuritySwPRNG     { 22  1  0 per_vpe Implementation }
    SecurityHwPRNG     { 22  2  0 per_vpe Implementation }
    SecurityScrambling { 22  3  0 per_vpe Implementation }
    Debug              { 23  0 32 per_tc  EJTAG }
    Debug2             { 23  6  0 per_tc  EJTAG }
    TraceControl       { 23  1 32 per_vpe PDTrace }
    TraceControl2      { 23  2 32 per_vpe PDTrace }
    UserTraceData      { 23  3  0 per_vpe PDTrace }
    UserTraceData1     { 23  3 32 per_vpe PDTrace }
    TraceBPC           { 23  4 32 per_vpe PDTrace }
    TraceIBPC          { 23  4  0 per_vpe PDTrace }
    TraceDBPC          { 23  5  0 per_vpe PDTrace }
    DEPC               { 24  0 32 per_vpe EJTAG }
    TraceControl3      { 24  2  0 per_vpe PDTrace }
    UserTraceData2     { 24  3 32 per_vpe PDTrace }
    PerfCtl0           { 25  0 32 per_core }
    PerfCnt0           { 25  1 32 per_core }
    PerfCtl1           { 25  2 32 per_core }
    PerfCnt1           { 25  3 32 per_core }
    PerfCtl2           { 25  4  0 per_core }
    PerfCnt2           { 25  5  0 per_core }
    PerfCtl3           { 25  6  0 per_core }
    PerfCnt3           { 25  7  0 per_core }
    ErrCtl             { 26  0 32 per_vpe }
    DErrCtl            { 26  0  0 per_vpe }
    IErrCtl            { 26  1  0 per_vpe }
    CacheErr           { 27  0  0 per_vpe }
    TagLo              { 28  0  0 per_vpe }
    ITagLo             { 28  0  0 per_vpe }
    DataLo             { 28  1  0 per_vpe }
    IDataLo            { 28  1  0 per_vpe }
    DTagLo             { 28  2  0 per_vpe }
    DDataLo            { 28  3  0 per_vpe }
    L23TagLo           { 28  4  0 per_vpe }
    L23DataLo          { 28  5  0 per_vpe }
    TagHi              { 29  0  0 per_vpe }
    ITagHi             { 29  0  0 per_vpe }
    IDataHi            { 29  1  0 per_vpe }
    DTagHi             { 29  2  0 per_vpe }
    DDataHi            { 29  3  0 per_vpe }
    L23DataHi          { 29  5  0 per_vpe }
    DataHi             { 29  1  0 per_vpe }
    ErrorEPC           { 30  0 32 per_vpe }
    DESAVE             { 31  0 32 per_vpe EJTAG }
    KScratch1          { 31  2  0 per_vpe }
    KScratch2          { 31  3  0 per_vpe }
    KScratch3          { 31  4  0 per_vpe }
    KScratch4          { 31  5  0 per_vpe }
    KScratch5          { 31  6  0 per_vpe }
    KScratch6          { 31  7  0 per_vpe }
}


# ====================================================================================
echo "    <CP0RegName> ?<new-value>?  (All CP0 registers now accessible by name.)"
# ====================================================================================

foreach { reg_name reg_info } [array get cp0_info] {
    
    # Note: using hard-coded info entry showing register size on this processor. (0 == does not exist.)
    if {[lindex $reg_info 2] == 32} {
        set procbody "
            variable cp0_info
            set reg_info \$cp0_info($reg_name)
            if {\$args !=  {} } {
                mips32 cp0 [lindex $reg_info 0] [lindex $reg_info 1] \$args
            } else {
                mips32 cp0 [lindex $reg_info 0] [lindex $reg_info 1]
            }
        "
        proc $reg_name {args} $procbody
    }
}

# ====================================================================================
echo "    cp0                         (dump all CP0 registers.)"
# ====================================================================================
proc cp0x {} {

    # Note: using hard-coded info entry showing register size on this processor. (0 == does not exist.)
    variable cp0_info

    foreach { reg_name reg_info } [array get cp0_info] {
        if {[lindex $reg_info 2] == 32} {
            echo -n "[format "%-16s" $reg_name]"
            flush stdout
            $reg_name
        }
    }
}

# ====================================================================================
# echo "leaving 'mAUC.tcl'"
# ====================================================================================
return ""
