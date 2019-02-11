# http://infocenter.arm.com/help/topic/com.arm.doc.ddi0464f/BABDIJAD.html
proc cache_a7_d_dump { } {
	for { set way 0  } { $way < 4 } { set way [expr $way + 1]} {

		# TODO: calculate index size
		for { set index 0  } { $index < 32 } { set index [expr $index + 1]} {

			set val [expr ($way << 30) | ($index << 6)]
			arm mcr 15 3 15 2 0 $val
			set tag0 [arm mrc 15 3 15 0 0]
			set tag1 [arm mrc 15 3 15 0 1]

			for { set word 0  } { $word < 8 } { set word [expr $word + 1]} {

				set val [expr $val | ($word << 3)]
				arm mcr 15 3 15 4 0 $val
				set dat0 [arm mrc 15 3 15 0 0]
				set dat1 [arm mrc 15 3 15 0 1]

    				echo [format "w:%i, i:%i, wr:%i tag0: 0x%08x; tag1: 0x%08x; data0: 0x%08x, data1: 0x%08x" $way $index $word $tag0 $tag1 $dat0 $dat1]

			}

		}


	}
}

# http://infocenter.arm.com/help/topic/com.arm.doc.ddi0464f/BABDFFFC.html
proc cache_a7_i_dump { } {
	for { set way 0  } { $way < 2 } { set way [expr $way + 1]} {

		# TODO: calculate index size
		for { set index 0  } { $index < 32 } { set index [expr $index + 1]} {

			set val [expr ($way << 31) | ($index << 5)]
			arm mcr 15 3 15 2 1 $val
			set tag [arm mrc 15 3 15 0 0]

			for { set word 0  } { $word < 8 } { set word [expr $word + 1]} {

				set val [expr $val | ($word << 2)]
				arm mcr 15 3 15 4 1 $val
				set dat0 [arm mrc 15 3 15 0 0]
				set dat1 [arm mrc 15 3 15 0 1]

    				echo [format "tag: 0x%08x; data0: 0x%08x, data1: 0x%08x" $tag $dat0 $dat1]

			}

		}


	}
}
