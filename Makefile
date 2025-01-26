obj-m := panel-sitronix-st7796s.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	sudo cp panel-sitronix-st7796s.ko /lib/modules/$(shell uname -r)/kernel/drivers/gpu/drm/panel
	sudo depmod -a
