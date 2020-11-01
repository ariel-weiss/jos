# jos
Jos: exokernel OS

## Installation
<p>
The only way I'm going to present here is throuh Docker.
<br>So first download docker from here.
<br>Then, open a terminal and make sure your in the /jos/ folder.
<br>Type these commands in order to run Jos image and activate Jos's shell:

```
docker build --tag jos:latest .
docker run -it jos:latest

make
make qemu-nox
```

<br>you can see all user files if you type 'ls' command.
<br>
have fun!
