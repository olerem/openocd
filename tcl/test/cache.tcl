source [find mem_helper.tcl]

proc cache_mrw {reg} {
	set ret [lindex [ocd_mdw $reg] 1]; sleep 0;
	return 0x$ret;
#	return [mrw $reg]
}

proc mww_with_test {reg val} {
	mww $reg $val

	set tmp [cache_mrw $reg]
	if { $val != $tmp } {
		echo [format "write error. Got: 0x%08x, expected: 0x%08x" $tmp $val]
	}
}

proc cache_inval_test {reg} {
	set pattern_1 0xaaaaaaaa
	set pattern_2 0x55555555
	set tmp 0

	halt
	# Disable auto cache handling
	cache auto 0

	# Write down some pattern and make sure it will go to the RAM
	mww_with_test $reg $pattern_1
	if [ catch { cache l1 d clean $reg } msg ] {
		return "ERROR: clean l1 cache"
	}
	if [ catch { cache l2x clean $reg } msg ] {
		return "ERROR: clean l2x cache"
	}

	if [ catch { cache l1 d inval $reg } msg ] {
		return "ERROR: inval l1 inval"
	}
	if [ catch { cache l2x inval $reg } msg ] {
		return "ERROR: inval l2x inval"
	}

	set tmp [cache_mrw $reg]
	if { $pattern_1 != $tmp } {
		echo [format "ERROR. Write to RAM over CPU. Got: 0x%08x, expected: 0x%08x" $tmp $pattern_1]
	} else {
		echo "OK   . Write to RAM over CPU"
	}

	# Write other patter. 
	mww_with_test $reg $pattern_2

	if [ catch { cache l1 d inval $reg } msg ] {
		return "ERROR: clean l1 inval"
	}
	# TODO, currently l1 d inval fails working. Fix it!!!!
	set tmp [cache_mrw $reg]
	if { $pattern_1 != $tmp } {
		echo [format "ERROR. Inval l1. Got: 0x%08x, expected: 0x%08x" $tmp $pattern_1]
	} else {
		echo "OK   . Inval l1"
	}

	if [ catch { cache l2x inval $reg } msg ] {
		return "ERROR: clean l2x inval"
	}
	set tmp [cache_mrw $reg]
	if { $pattern_1 != $tmp } {
		echo [format "ERROR. Inval l2x. Got: 0x%08x, expected: 0x%08x" $tmp $pattern_1]
	} else {
		echo "OK   . Inval l2x"
	}
}
