// 家族計画 ～追憶～ Switch 移植 · 阶段 1 验证程序
//
// 目的：跑通 devkitPro → libnx → .nro 全链路，在真机上验证：
//   1. console 输出正常
//   2. romfs 资源读取正常
//   3. 手柄按键事件正常
//   4. 存档目录可写
//
// 阶段 2（SDL2 渲染 + ISF VM 引擎核心）在此验证通过后接入。

#include <stdio.h>
#include <string.h>
#include <switch.h>

#define APP_VERSION "0.1.0-stage1"

static char g_line[256];

static void draw(void)
{
    printf(CONSOLE_ESC(2J) CONSOLE_ESC(1;1H));
    printf("\x1b[32m==================================================\x1b[0m\n");
    printf("   家族計画 ～追憶～  Switch Port\n");
    printf("   Stage 1 - build environment verification\n");
    printf("   version " APP_VERSION "\n");
    printf("\x1b[32m==================================================\x1b[0m\n\n");
    printf("  [A] test pad input\n");
    printf("  [B] test romfs read\n");
    printf("  [X] test save write\n");
    printf("  [+] exit\n\n");
    printf("  log: %s\n", g_line);
    printf("\n\x1b[90m  engine next: ISF VM + GGD/GGA decode + SDL2\x1b[0m\n");
    consoleUpdate(NULL);
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    // console 初始化（640x480 逻辑在 stage2 用 framebuffer 实现）
    consoleInit(NULL);
    snprintf(g_line, sizeof g_line, "boot ok, version " APP_VERSION);
    draw();

    // romfs 挂载
    Result rc = romfsInit();
    bool romfs_ok = R_SUCCEEDED(rc);
    if (!romfs_ok) snprintf(g_line, sizeof g_line, "romfsInit failed: 0x%x", rc);

    bool running = true;
    while (running && appletMainLoop())
    {
        PadState pad;
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad);

        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) {
            running = false;
        }
        if (kDown & HidNpadButton_A) {
            snprintf(g_line, sizeof g_line, "pad A ok - input works!");
            draw();
        }
        if (kDown & HidNpadButton_B && romfs_ok) {
            FILE *f = fopen("romfs:/readme.txt", "r");
            if (f) {
                char buf[128] = {0};
                size_t n = fread(buf, 1, sizeof buf - 1, f);
                fclose(f);
                snprintf(g_line, sizeof g_line, "romfs ok, read %d bytes: %.40s", (int)n, buf);
            } else {
                snprintf(g_line, sizeof g_line, "romfs open FAILED");
            }
            draw();
        }
        if (kDown & HidNpadButton_X) {
            FILE *f = fopen("sdmc:/switch/kzn_stage1.txt", "w");
            if (f) {
                fprintf(f, "kzn stage1 save test ok\n");
                fclose(f);
                snprintf(g_line, sizeof g_line, "save write ok -> sdmc:/switch/kzn_stage1.txt");
            } else {
                snprintf(g_line, sizeof g_line, "save write FAILED (need dir)");
            }
            draw();
        }

        consoleUpdate(NULL);
    }

    if (romfs_ok) romfsExit();
    consoleExit(NULL);
    return 0;
}
