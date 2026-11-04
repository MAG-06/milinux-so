#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/reboot.h>

#define EQUIPO "Equipo 03"

int main(int argc, char *argv[], char *envp[]) {
    printf("\n========================================\n");
    printf("  %s - Mi primer init\n", EQUIPO);
    printf("========================================\n");
    printf("PID: %d   PPID: %d\n", getpid(), getppid());
    printf("Argumentos recibidos del kernel (argc=%d):\n", argc);
    for (int i = 0; i < argc; i++)
        printf("  argv[%d] = %s\n", i, argv[i]);
    printf("Variables de entorno recibidas del kernel:\n");
    for (char **e = envp; *e != NULL; e++)
        printf("  %s\n", *e);
    fflush(stdout);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "medir") == 0) {
            printf("Modo medir: reiniciando para que QEMU termine.\n");
            fflush(stdout);
            sync();
            reboot(RB_AUTOBOOT);
        }
        if (strcmp(argv[i], "salir") == 0) {
            printf("init va a terminar... observe lo que hace el kernel.\n");
            fflush(stdout);
            return 0;
        }
    }

    printf("init sigue vivo. Presione Ctrl+a y luego x para cerrar QEMU.\n");
    fflush(stdout);
    for (;;)
        pause();
}
