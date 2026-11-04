# milinux-so

Proyecto **"Construye tu propio Sistema Operativo Linux"** – Sistemas Operativos, Ingeniería de Software.

**Autor:** Mateo Albornoz Gil (MAG-06) · Trabajo individual (sufijo de equipo usado: `-equipo03`)

## Estado

| Parte | Tema | Estado | Bitácora |
|---|---|---|---|
| 1 | Kernel y arranque mínimo | ✅ Completa | [bitacora/parte1.md](bitacora/parte1.md) |
| 2–7 | — | Pendiente | — |

## Parte 1 en resumen

- Kernel **Linux 6.18.60** (longterm) compilado con `defconfig` y `tinyconfig`, ambos con el sufijo `-equipo03`.
- `init` propio en C, compilado estático, empaquetado en un initramfs y arrancado en QEMU como **PID 1**.
- Comparación entre los dos kernels:

| Medida | defconfig | tinyconfig |
|---|---|---|
| Tamaño bzImage | 14 MB | 977 KB |
| Opciones `=y` | 1635 | 527 |
| Compilación | 11 min 24 s | 2 min 12 s |
| Arranque hasta PID 1 (mediana) | 6,01 s | 2,78 s |

## Estructura

```
milinux-so/
├── bitacora/     # parte1.md, capturas/ y las configuraciones (config-default, config-tiny)
├── logs/         # logs de compilación, de cada arranque en QEMU y de las mediciones
├── src/init.c    # el primer proceso (PID 1)
├── initramfs/    # contenido del initramfs (/init); dev/console se crea con mknod
└── .gitignore    # fuentes del kernel, build-* y archivos generados
```

## Entorno

Ubuntu 26.04.1 LTS en VirtualBox (6 CPU, 8 GB RAM) · gcc 15.2.0 · QEMU (`qemu-system-x86_64`).

> Las fuentes del kernel y las carpetas `build-*` no se suben (están en `.gitignore`). Se reconstruyen siguiendo los pasos de la guía.
