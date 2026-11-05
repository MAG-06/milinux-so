# Bitácora - Parte 2: Espacio de usuario y dispositivos

**Estudiante:** Mateo Albornoz Gil (Mag) · **Equipo:** 03
**Asignatura:** Sistemas Operativos - Ingeniería de Software
**Kernel usado:** el mismo `build-default` de la Parte 1 (6.18.60-equipo03), no se recompiló · **BusyBox:** 1.37.0 · **Fecha:** 4 de noviembre de 2026

> El 03 se deja como en la Parte 1: el profe dijo que el número de equipo no importa (hostname `milinux-03`).

Verificación previa: `CONFIG_BINFMT_SCRIPT=y`, `CONFIG_DEVTMPFS=y`, `CONFIG_PROC_FS=y` en `build-default/.config`.

## Actividad 1 · Compilar BusyBox (E1: `logs/build-busybox.log`)
- `make defconfig` + `sed`: el `grep` mostró `CONFIG_STATIC=y` y `# CONFIG_TC is not set`.
- `make -j6`: **42,9 s** reales (`logs/time-busybox.txt`), luego `make CONFIG_PREFIX=../rootfs install`.
- `file busybox`: `ELF 64-bit LSB executable, x86-64, ... statically linked, ... stripped`. `ls -lh busybox`: **2.5M**.

**P1. ¿Cuánto pesa el archivo busybox y cuántos enlaces simbólicos creó la instalación? Compare con el tamaño de su init de la Parte 1.**
`busybox` pesa **2.526.552 bytes (2,5 MB)**. La instalación creó **402 enlaces simbólicos** (`find rootfs -type l | wc -l`); después de `rm -f linuxrc` quedan **401**. El init de la Parte 1 (`src/init`) pesa **821.400 bytes (~803 KB)**: BusyBox es unas 3 veces más grande, pero ese único binario trae cientos de comandos (shell, `mount`, `ps`, `ls`, `mknod`, `dd`...), mientras que el init solo hacía una cosa.

**P2. ¿A dónde apunta rootfs/bin/ls? Explique cómo un solo programa puede comportarse como ls, cp y mount.**
`rootfs/bin/ls -> busybox` (igual que `cat`, `cp`, `ash`, `cttyhack`... en `ls -l rootfs/bin | head -15`). BusyBox es **multicall**: cuando se ejecuta, mira con qué nombre lo llamaron (`argv[0]`, que es el nombre del enlace) y corre la función de ese applet. Si lo llaman por `ls` hace de `ls`, si lo llaman por `mount` hace de `mount`. Así el código común (libc estática, manejo de argumentos) está una sola vez.

## Actividad 2 · Jerarquía y `/init` (E2: `rootfs/init`)
- `mkdir -p dev proc sys etc tmp root`, `sudo mknod -m 600 dev/console c 5 1`, `/init` de la guía con comentarios propios, `chmod +x init`, `rm -f linuxrc`.
- `rootfs.cpio.gz`: **1.329.967 bytes (1,3 MB)**.

**P3. Explique para qué sirve cada carpeta creada en 2.1. ¿Cuáles de ellas estarán vacías en el archivo rootfs.cpio.gz y se llenarán solo al arrancar?**
- `dev`: archivos de dispositivo; ahí se monta devtmpfs. Trae `dev/console` para que el kernel pueda escribir antes de que `/init` monte nada.
- `proc`: punto de montaje de procfs (procesos y estado del kernel).
- `sys`: punto de montaje de sysfs (modelo de dispositivos).
- `etc`: archivos de configuración del sistema.
- `tmp`: archivos temporales.
- `root`: carpeta personal del usuario root.

En el `.cpio.gz` van **vacías `proc`, `sys`, `etc`, `tmp` y `root`**, y `dev` solo lleva `console`. `proc`, `sys` y `dev` se llenan al arrancar porque el kernel genera su contenido cuando `/init` los monta. `etc`, `tmp` y `root` siguen vacías hasta que alguien escriba ahí.

**P4. ¿Por qué el script usa exec en la última línea? Prediga qué PID tendrá la terminal.**
`exec` **reemplaza** el proceso del script por el shell, sin crear un proceso hijo. El kernel lanza `/init` como PID 1, así que el shell hereda ese PID. **Predicción: PID 1.** Sin `exec`, el shell sería un hijo con otro PID y, si el script terminaba, el PID 1 moriría y el kernel entraría en pánico.

## Actividad 3 · Arrancar y comprobar quién es PID 1 (E3: `logs/arranque-busybox.log`)
Salida del arranque: `=== Mi Linux 03: espacio de usuario con BusyBox ===` y `Kernel: 6.18.60-equipo03   Procesos: 53`. `hostname` → `milinux-03`. `uname -a` → `Linux milinux-03 6.18.60-equipo03 #1 SMP PREEMPT_DYNAMIC ...`.

**P5. ¿Qué PID muestra echo $$? ¿Coincide con su predicción de P4? ¿Qué procesos aparecen en ps y cuáles tienen el nombre entre corchetes?**
`echo $$` mostró **1**, igual que la predicción de P4. En `ps` aparecen el PID 1 `sh`, el propio `ps` (PID 66) y unos 50 procesos más entre corchetes: `[kthreadd]`, `[ksoftirqd/0]`, `[rcu_preempt]`, `[migration/0]`, `[kdevtmpfs]`, `[kswapd0]`, `[kworker/...]`, `[scsi_eh_0]`, etc. Los que tienen **corchetes son hilos del kernel**: no vienen de un programa en disco ni tienen línea de comandos, así que `ps` muestra su nombre entre `[ ]`. Los únicos procesos de usuario son `sh` y `ps`.

**P6. ¿Qué mensaje apareció al escribir exit? Relaciónelo con la Actividad 5 de la Parte 1.**
`Kernel panic - not syncing: Attempted to kill init! exitcode=0x00000000`. Es el mismo panic de la Actividad 5 de la Parte 1, cuando el init propio terminaba. El PID 1 no puede terminar porque el kernel lo necesita para adoptar a los procesos huérfanos. Aquí el PID 1 era el shell (por el `exec`), así que salir del shell equivale a matar a init.

**P7. Copie la salida de mount e identifique los tres sistemas de archivos que montó su script.**
```
rootfs on / type rootfs (rw,size=106984k,nr_inodes=26746)
proc on /proc type proc (rw,relatime)
sysfs on /sys type sysfs (rw,relatime)
devtmpfs on /dev type devtmpfs (rw,relatime,size=106984k,...)
```
Los tres que montó el script son **proc** en `/proc`, **sysfs** en `/sys` y **devtmpfs** en `/dev`. El `rootfs` en `/` no lo montó el script: es el initramfs que el kernel desempaquetó en RAM.

## Actividad 4 · Explorar /proc y /sys (E4)
**P8. ¿Qué mostró ps después de umount /proc? Explique de dónde obtiene ps la información de los procesos.**
Después de `umount /proc`, `ps` mostró **solo el encabezado** `PID USER TIME COMMAND`, sin ningún proceso, y `ls /proc` salió vacío. Al volver a montarlo (`mount -t proc proc /proc`), `ps` volvió a listar `1 sh`, `2 [kthreadd]`... `ps` no tiene la información por su cuenta: **lee las carpetas numéricas de `/proc`** (`/proc/1/status`, `/proc/1/cmdline`...), que el kernel genera al momento. Sin procfs montado no tiene de dónde leer.

**P9. En /proc/1/status, identifique Name, State, PPid y VmRSS. ¿Qué programa es el PID 1 ahora y cuánta memoria física usa?**
`Name: sh` · `State: R (running)` (en la segunda lectura con `head -12`: `S (sleeping)`, esperando que se escriba algo) · `PPid: 0` · `VmRSS: 1812 kB`. El PID 1 ahora es **`sh` de BusyBox** (ya no el init de la Parte 1) y usa **1.812 kB (~1,8 MB) de memoria física**. `PPid: 0` porque no tiene padre: lo creó el kernel.

**P10. ls -l /proc/meminfo muestra tamaño 0, pero cat muestra contenido. Explique por qué.**
`ls -l /proc/meminfo` → `-r--r--r-- 1 0 0 0 ... /proc/meminfo` (tamaño **0**). `/proc` es un sistema de archivos **virtual**: el archivo no existe en disco ni en RAM con datos guardados. Cuando `cat` lo lee, el kernel **genera el texto en ese momento** con los valores actuales de memoria. Como el contenido no existe antes de leerlo, el kernel no sabe el tamaño y reporta 0.

**P11. Compare MemTotal de /proc/meminfo con los 256 MB que le dio a QEMU. ¿Por qué no son iguales?**
`MemTotal: 220076 kB` (~215 MB), contra los 256 MB (262.144 kB) de `-m 256M`, o sea unos **41 MB menos**. `MemTotal` cuenta solo la RAM que el kernel puede repartir. El resto queda reservado: zonas que el BIOS/firmware marca como no usables, el código y los datos del propio kernel, y estructuras que crea al arrancar (tablas de páginas, `struct page` de cada página, etc.).
Otros datos de la 4.2: `/proc/cmdline` = `console=ttyS0 quiet`, `grep -c processor /proc/cpuinfo` = **1** (QEMU sin `-smp` da 1 CPU, `QEMU Virtual CPU version 2.5+`), `/proc/uptime` = `30.86 25.05` (segundos encendido / segundos ocioso), `/proc/filesystems`: `sysfs`, `tmpfs`, `proc`, `cgroup2`, `devtmpfs`, `ramfs`, `devpts`, `ext2/3/4`, `vfat`, `iso9660`, `nfs`, `9p`... (`nodev` = no necesita disco).

**P12. ¿Qué interfaces de red y qué dispositivos de bloque aparecen en /sys? ¿Quién los creó, si usted no instaló nada?**
- Red (`/sys/class/net`): **`eth0`, `lo` y `sit0`**. `eth0` tiene MAC **52:54:00:12:34:56** (la que QEMU pone por defecto).
- Bloque (`/sys/block`): **`loop0`–`loop7` y `sr0`** (el lector de CD virtual). No hay disco porque a QEMU no se le pasó `-hda`.

Los creó **el kernel**. Al arrancar detectó el hardware virtual de QEMU (tarjeta de red, CD-ROM) con los drivers que ya traía compilados en `defconfig`, y creó solo `lo`, `sit0` y los `loop`. sysfs solo muestra ese modelo de dispositivos que el kernel armó; yo no instalé nada. `/sys/devices/system/cpu` muestra solo `cpu0`.

## Actividad 5 · Archivos de dispositivo: mknod frente a devtmpfs (E4)
- 5.1: `ls -l /dev | head -20` muestra nodos `c` (carácter: `console 5,1`, `full 1,7`, `kmsg 1,11`, `hpet 10,228`...) y `b` (bloque: `loop0`–`loop7` con mayor 7). `/dev/null` = `c 1,3`, `/dev/zero` = `c 1,5`, `/dev/ttyS0` = `c 4,64`. En `/proc/devices` el mayor 1 es `mem`, el 4 es `tty`/`ttyS`, el 5 es `/dev/tty`, `/dev/console` y `/dev/ptmx`, y en bloque el 7 es `loop`, el 8 `sd` y el 11 `sr`.
- 5.2: `umount -l /dev` → `ls -l /dev` muestra solo `console`. `echo "esto desaparece" > /dev/null` no muestra nada; `echo "hola desde mi propio dispositivo" > /dev/mi_consola` lo imprimió en pantalla.
- 5.3: `mount -t devtmpfs devtmpfs /dev` → `ls /dev | wc -l` vuelve a **111**.

**P13. ¿Cuántos archivos había en /dev con devtmpfs y cuántos quedaron al desmontarlo? ¿De dónde salió el que quedó?**
Con devtmpfs había **111**. Al desmontarlo quedó **1**, `console` (`c 5,1`). Ese salió del **propio rootfs**: lo creé en la Actividad 2 con `sudo mknod -m 600 dev/console c 5 1` y va dentro de `rootfs.cpio.gz`. devtmpfs estaba montado encima y lo tapaba; al quitarlo aparece la carpeta `dev` original del initramfs.

**P14. /dev/mi_consola no se llama console, pero el texto apareció en la pantalla. Explique qué identifica realmente a un dispositivo.**
Lo que identifica al dispositivo es **el tipo (`c` carácter o `b` bloque) y el par de números mayor,menor**, no el nombre. `mi_consola` se creó como `c 5 1`, los mismos números que `console`. Cuando se escribe en él, el kernel mira el mayor 5 y el menor 1 y le pasa los datos al driver de la consola, que los muestra en pantalla. El nombre es solo una etiqueta para nosotros. Igual pasó con `null` (`c 1 3`): el driver `mem` descartó el texto.

**P15. Compare crear dispositivos con mknod y usar devtmpfs. ¿Qué problema tendría un sistema con cientos de dispositivos si los creara a mano?**
Con `mknod` hay que crear cada nodo a mano y saber su tipo y sus números mayor y menor. Además, los nodos se quedan aunque el hardware no exista. Con **devtmpfs el kernel crea solo un nodo por cada dispositivo que detecta**, con los números correctos, y los agrega o quita cuando se conecta o desconecta algo. Con cientos de dispositivos a mano habría que:
- conocer cientos de pares de números;
- escribir y mantener un script enorme;
- crear nodos de hardware que tal vez no esté;
- equivocarse fácil en un número y terminar escribiendo en el dispositivo equivocado;
- y no se podría conectar nada en caliente (USB, discos) sin volver a correr `mknod`.

## Actividad 6 · Entrada/salida e interrupciones (E5)
- 6.1: `grep ttyS0 /proc/interrupts` → `4: 2041 IO-APIC 4-edge ttyS0`. Se presionaron 12 teclas (`asdfghjkl123`) sin ejecutar, se borraron, y otra vez → `4: 2202 IO-APIC 4-edge ttyS0`.
- 6.2: ver abajo. Cierre con `poweroff -f` → `reboot: Power down` y QEMU se cerró solo.

**P16. ¿Cuánto aumentó el contador de ttyS0 al presionar teclas? ¿En qué línea IRQ está? Explique qué hace el kernel en cada interrupción.**
Pasó de **2041 a 2202: aumentó 161**. ttyS0 está en la **IRQ 4** (`IO-APIC 4-edge`). Cada tecla genera interrupciones: una cuando el carácter llega al UART (recepción), otra cuando el shell hace eco del carácter y lo devuelve por el puerto (transmisión), y lo mismo con cada borrado y con el comando. En cada interrupción la CPU deja lo que estaba haciendo y salta al manejador del driver serie (8250). El driver:
1. lee el registro del UART para ver qué pasó;
2. si llegó un dato, lo copia al buffer de la tty para que el shell lo lea (y manda el siguiente byte si estaba transmitiendo);
3. confirma la interrupción y suma 1 al contador de `/proc/interrupts`;
4. le devuelve la CPU a lo que estaba corriendo.

**P17. ¿Qué velocidad reportó el primer dd? ¿Por qué es tan alta, si no hay disco involucrado?**
`209715200 bytes (200.0MB) copied, 0.075521 seconds, 2.6GB/s` (en la primera corrida dio 2.3GB/s). Es tan alta porque **no hay hardware de por medio**: `/dev/zero` y `/dev/null` son drivers que viven dentro del kernel, en memoria. Leer de `/dev/zero` solo llena el buffer con ceros y escribir en `/dev/null` solo lo descarta. Todo es copiar memoria y hacer llamadas al sistema, sin esperar a un disco ni a interrupciones, así que la velocidad depende solo de la CPU y la RAM.

**P18. /tmp no tiene disco detrás. ¿Dónde se guardaron los 20 MB del segundo dd? Use MemFree para justificarlo.**
Se guardaron en la **RAM**. `/tmp` está dentro del rootfs, que es un sistema de archivos en memoria (el initramfs desempaquetado). `ls -lh /tmp/archivo` = `20.0M` (el `dd` iba a 100.9MB/s). Con `head -3 /proc/meminfo`:
| Momento | MemFree | MemAvailable |
|---|---|---|
| antes del 2.º `dd` | 205.784 kB | 202.672 kB |
| con `/tmp/archivo` | 184.676 kB | 181.576 kB |
`MemFree` bajó **21.108 kB ≈ 20,6 MB**, justo el tamaño del archivo (20 MB = 20.480 kB más un poco de estructuras del sistema de archivos). Eso prueba que el archivo ocupa RAM. Justo después del `rm /tmp/archivo`, `head -3` seguía mostrando `MemFree: 184676 kB`: el kernel no devolvió esas páginas de inmediato en esa lectura. En la primera corrida también se vio `Shmem` subir de 2.476 a 22.956 kB (+20.480 kB exactos).

## Trabajo autónomo
**1. Árbol de `rootfs`** (`find rootfs -maxdepth 2 -type d`):
```
rootfs            raíz del sistema: lo que se ve como / dentro de QEMU
rootfs/bin        enlaces de comandos básicos de BusyBox (ls, sh, mount, cat...)
rootfs/sbin       enlaces de comandos de administración (mknod, poweroff, ifconfig...)
rootfs/usr        segunda jerarquía de programas, como en un Linux normal
rootfs/usr/bin    más enlaces de comandos de usuario (wc, head, find...)
rootfs/usr/sbin   más enlaces de administración
rootfs/dev        aquí se monta devtmpfs; trae dev/console creado con mknod
rootfs/proc       punto de montaje de procfs (vacía en el cpio)
rootfs/sys        punto de montaje de sysfs (vacía en el cpio)
rootfs/etc        configuración del sistema (vacía por ahora)
rootfs/tmp        archivos temporales, viven en RAM
rootfs/root       carpeta personal de root (vacía)
```

**2. Los tres montajes de `/init`:**
| Sistema de archivos | Qué información expone | Qué comando dejó de funcionar o funcionó gracias a él |
|---|---|---|
| `proc` en `/proc` | procesos (`/proc/1/status`), memoria (`meminfo`), CPU, interrupciones, línea de arranque | `ps` quedó vacío al hacer `umount /proc`; gracias a él funcionan `meminfo`, `/proc/interrupts` y el conteo de procesos del `/init` |
| `sysfs` en `/sys` | modelo de dispositivos: interfaces de red, dispositivos de bloque, CPUs, drivers | `ls /sys/class/net`, `cat /sys/class/net/eth0/address`, `ls /sys/block` |
| `devtmpfs` en `/dev` | un archivo por cada dispositivo que detectó el kernel | sin él `/dev` quedó solo con `console`; gracias a él funcionan `dd` con `/dev/zero` y `/dev/null`, y la terminal en `/dev/ttyS0` |

**3. Comentarios del script:** están en `rootfs/init`, un comentario por instrucción con mis palabras.

## Errores encontrados
| Mensaje | Causa | Solución |
|---|---|---|
| Primer arranque: `Kernel panic - not syncing: IO-APIC + timer doesn't work!` (después de `..MP-BIOS bug: 8254 timer not connected to IO-APIC`) | QEMU sin aceleración (TCG) dentro de VirtualBox: el chequeo del timer del IO-APIC a veces falla por temporización | cerrar con `Ctrl+a x` y volver a lanzar el mismo comando; arrancó normal al segundo intento y en los siguientes (no se cambió nada) |

## Capturas (`bitacora/capturas/`)
- **E1:** `e1-compilacion-busybox.jpg`, `e1-busybox-estatico-symlinks.jpg`, `e1-ls-rootfs-bin.jpg`
- **E2:** `e2-rootfs-dev-console.jpg`
- **E3:** `e3-error-ioapic-primer-arranque.jpg`, `e3-pid1-hostname-mount.jpg`, `e3-ps.jpg`, `e3-exit-panic.jpg`
- **E4 (Act. 4):** `e4-umount-proc-ps.jpg`, `e4-ls-proc-vacio.jpg`, `e4-lecturas-proc.jpg`, `e4-proc-guia-4.2.jpg`, `e4-lecturas-sys.jpg`
- **E4 (Act. 5):** `e4-dev-devtmpfs.jpg`, `e4-ls-dev-head20.jpg`, `e4-proc-devices.jpg`, `e4-mknod-vs-devtmpfs.jpg`, `e4-mknod-guia-5.2.jpg`
- **E5:** `e5-interrupts-antes.jpg`, `e5-interrupts-despues.jpg`, `e5-dd-meminfo.jpg`, `e5-dd-guia-6.2.jpg`, `e5-poweroff.jpg`, `e5-arbol-rootfs.jpg`
