// 家族計画 ～追憶～ Switch 移植 · Stage 2 引擎（日文版 MVP）
// SDL2 渲染 + SDL2_ttf 日文文字 + 手柄输入
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOGIC_W 640
#define LOGIC_H 480
#define FONT_SIZE 22

static SDL_Window   *g_win = NULL;
static SDL_Renderer *g_ren = NULL;
static TTF_Font     *g_font = NULL;

// 剧本缓冲：每行一个事件
static char **g_lines = NULL;
static int   g_line_count = 0;
static int   g_pos = 0;          // 当前事件索引

static void die(const char *msg) {
    FILE *f = fopen("sdmc:/switch/kzn/engine.log", "w");
    if (f) { fprintf(f, "FATAL: %s\n", msg); fclose(f); }
}

// 读取整个 romfs 文件到内存
static char *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f);
    buf[sz] = 0;
    fclose(f);
    if (out_len) *out_len = sz;
    return buf;
}

// 加载剧本，按行切分
static void load_script(const char *path) {
    size_t len = 0;
    char *data = read_file(path, &len);
    if (!data) { die("script open fail"); return; }
    int n = 0;
    for (size_t i = 0; i < len; i++) if (data[i] == '\n') n++;
    g_lines = (char **)malloc(sizeof(char *) * (n + 2));
    g_line_count = 0;
    char *p = data;
    while (p && *p) {
        char *nl = strchr(p, '\n');
        if (nl) *nl = 0;
        if (*p) g_lines[g_line_count++] = p;
        p = nl ? nl + 1 : NULL;
    }
    g_lines[g_line_count] = NULL;
}

// 找到当前位置之后的下一条 T: 文本事件
static const char *next_text(void) {
    while (g_pos < g_line_count) {
        char *line = g_lines[g_pos];
        g_pos++;
        if (line[0] == 'T' && line[1] == ':') {
            return line + 2;
        }
    }
    return NULL;
}

// 渲染文本（简单换行：每 30 字符一行，最多 3 行）
static void render_text(const char *text) {
    SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
    SDL_RenderClear(g_ren);

    SDL_Rect box = {20, LOGIC_H - 140, LOGIC_W - 40, 120};
    SDL_SetRenderDrawColor(g_ren, 20, 20, 40, 255);
    SDL_RenderFillRect(g_ren, &box);

    char line[256];
    const char *p = text;
    int y = box.y + 10;
    int row = 0;
    while (*p && row < 3) {
        int cnt = 0;
        while (p[cnt] && cnt < 30) cnt++;
        memcpy(line, p, cnt);
        line[cnt] = 0;
        SDL_Surface *surf = TTF_RenderUTF8_Blended(g_font, line, (SDL_Color){255,255,255,255});
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(g_ren, surf);
            SDL_Rect dst = {box.x + 10, y, surf->w, surf->h};
            SDL_RenderCopy(g_ren, tex, NULL, &dst);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
            y += surf->h + 4;
        }
        p += cnt;
        row++;
    }

    SDL_RenderPresent(g_ren);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    romfsInit();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        die("SDL_Init fail");
        return 1;
    }
    if (TTF_Init() < 0) {
        die("TTF_Init fail");
        return 1;
    }

    g_win = SDL_CreateWindow("kzn", 0, 0, 1280, 720, SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!g_win) { die("window fail"); return 1; }
    g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED);
    if (!g_ren) { die("renderer fail"); return 1; }
    SDL_RenderSetLogicalSize(g_ren, LOGIC_W, LOGIC_H);

    g_font = TTF_OpenFont("romfs:/font.ttf", FONT_SIZE);
    if (!g_font) { die("font fail"); return 1; }

    load_script("romfs:/script.txt");

    const char *cur = next_text();
    if (cur) render_text(cur);

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_JOYBUTTONDOWN) {
                int b = e.jbutton.button;
                if (b == 0) {
                    cur = next_text();
                    if (cur) render_text(cur);
                    else running = 0;
                }
                if (b == 10) running = 0;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_SPACE || e.key.keysym.sym == SDLK_RETURN) {
                    cur = next_text();
                    if (cur) render_text(cur);
                    else running = 0;
                }
                if (e.key.keysym.sym == SDLK_ESCAPE) running = 0;
            }
        }
        SDL_Delay(16);
    }

    TTF_CloseFont(g_font);
    SDL_DestroyRenderer(g_ren);
    SDL_DestroyWindow(g_win);
    TTF_Quit();
    SDL_Quit();
    romfsExit();
    return 0;
}
