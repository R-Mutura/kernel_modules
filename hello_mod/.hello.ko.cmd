cmd_/home/pi/Desktop/modules_test/hello_mod/hello.ko := ld -r  -EL -T ./scripts/module-common.lds -T ./arch/arm/kernel/module.lds  --build-id  -o /home/pi/Desktop/modules_test/hello_mod/hello.ko /home/pi/Desktop/modules_test/hello_mod/hello.o /home/pi/Desktop/modules_test/hello_mod/hello.mod.o ;  true
