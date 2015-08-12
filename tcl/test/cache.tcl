source [find mem_helper.tcl]

proc mww_with_test {reg val} {
	mww $reg $val

	set tmp [mrw $reg]
	if { $val != $tmp } {
		echo [format "write error. Got: 0x%08x, expected: 0x%08x" $tmp $val]
	}
}

proc my_cache_clean_test {} {
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
	cache l1 d clean $def_addr
	cache l2x clean $def_addr

	cache l1 d inval $def_addr
	cache l2x inval $def_addr

	set tmp [mrw $def_addr]
	if { $pattern_1 != $tmp } {
		echo [format "Write to RAM - ERROR. Got: 0x%08x, expected: 0x%08x" $tmp $pattern_1]
	} else {
		echo "Write to RAM - OK"
	}

	# Write other patter. 
	mww_with_test $def_addr $pattern_2

	cache l1 d inval $def_addr
	# TODO, currently l1 d inval is not working. Fix it!!!!
	set tmp [mrw $def_addr]
	if { $pattern_1 != $tmp } {
		echo [format "Inval l1 - ERROR. Got: 0x%08x, expected: 0x%08x" $tmp $pattern_1]
	} else {
		echo "Inval l1 - OK"
	}

	cache l2x inval $def_addr
	set tmp [mrw $def_addr]
	if { $pattern_1 != $tmp } {
		echo [format "Inval l2x - ERROR. Got: 0x%08x, expected: 0x%08x" $tmp $pattern_1]
	} else {
		echo "Inval l2x - OK"
	}
}
