</$objtype/mkfile

default:VQ: all

all install clean nuke test:VQ:
	@{ cd src && mk $target }
