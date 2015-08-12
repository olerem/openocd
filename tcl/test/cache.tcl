source [find mem_helper.tcl]

proc mww_with_test {reg val} {
	mww $reg $val

	set tmp [mrw $reg]
	if { $val != $tmp } {
		echo [format "write error. Got: 0x%08x, expected: 0x%08x" $tmp $val]
	}
}

proc my_clean_test {} {
	set def_addr 0x805cabd0
	set pattern_1 0xaaaaaaaa
	set pattern_2 0x55555555
	set tmp 0
	set targetname [target current]

	halt
	# Disable auto cache handling
	cache auto 0

	# Write down some pattern and make sure it will go to the RAM
	mww_with_test $def_addr $pattern_1
	if [ catch { cache l1 d clean $def_addr } msg ] {
		return "ERROR: clean l1 cache"
	}
	if [ catch { cache l2x clean $def_addr } msg ] {
		return "ERROR: clean l2x cache"
	}

	if [ catch { cache l1 d inval $def_addr } msg ] {
		return "ERROR: inval l1 inval"
	}
	if [ catch { cache l2x inval $def_addr } msg ] {
		return "ERROR: inval l2x inval"
	}

	set tmp [mrw $def_addr]
	if { $pattern_1 != $tmp } {
		echo [format "ERROR. Write to RAM. Got: 0x%08x, expected: 0x%08x" $tmp $pattern_1]
	} else {
		echo "OK   . Write to RAM"
	}

	# Write other patter. 
	mww_with_test $def_addr $pattern_2

	if [ catch { cache l1 d inval $def_addr } msg ] {
		return "ERROR: clean l1 inval"
	}
	# TODO, currently l1 d inval is not working. Fix it!!!!
	set tmp [mrw $def_addr]
	if { $pattern_1 != $tmp } {
		echo [format "ERROR. Inval l1. Got: 0x%08x, expected: 0x%08x" $tmp $pattern_1]
	} else {
		echo "OK   . Inval l1"
	}

	if [ catch { cache l2x inval $def_addr } msg ] {
		return "ERROR: clean l2x inval"
	}
	set tmp [mrw $def_addr]
	if { $pattern_1 != $tmp } {
		echo [format "ERROR. Inval l2x. Got: 0x%08x, expected: 0x%08x" $tmp $pattern_1]
	} else {
		echo "OK   . Inval l2x"
	}
}
