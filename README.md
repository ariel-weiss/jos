# Jos Operating System
This is ![Shaya's](https://github.com/yishayahu) and mine implementation of the Jos operating system.

The main skeleton was provided by Technion's OS Engineering class, inspaired from ![MIT class](https://pdos.csail.mit.edu/6.828/2016/overview.html).

Instructions to the makefile and more can be found ![here](https://pdos.csail.mit.edu/6.828/2016/labguide.html).

## Build & Run
<p>
The best way to use Jos without over complicating things is Docker :whale: .
  
- In order to run Jos container you'll need docker installed:

  * [Windows](https://docs.docker.com/windows/started)
  * [OS X](https://docs.docker.com/mac/started/)
  * [Linux](https://docs.docker.com/linux/started/)

- Clone this repository into /jos/ folder
- Open terminal and type these commands:

  ```shell
  docker build --tag jos:latest .
  docker run -it jos:latest
  ```
Now you have started docker contianer with Jos. 

One option to start fiddle with Jos is with normal qemu make:
<br><small>(make sure your in /jos/ folder)</small>
  ```bash
  make
  make qemu-nox
  ```

You can see all user files (from jos/user/ folder) if you type the 'ls' command.

Have fun!

---
### If docker isn't your cap of tea
You can try and build jos on your machine.
In this case, the prequisities are
- 64bit Ubuntu 14+
- latest QEMU installed

Good luck :v:
