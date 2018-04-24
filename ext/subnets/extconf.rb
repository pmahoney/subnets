require 'mkmf'

have_header('ctype.h')
have_header('stdint.h')

create_makefile('subnets')
