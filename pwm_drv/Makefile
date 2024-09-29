obj-m +=  pwm_drv.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lin/modules/$(shell uname -r)/build M=$(PWD) clean
