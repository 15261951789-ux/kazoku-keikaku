// 家族計画 ～追憶～ Switch 移植 · Stage 2 引擎（日文版 + 背景图）
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define LOGIC_W 640
#define LOGIC_H 480
#define FONT_SIZE 22

static SDL_Window   *g_win = NULL;
static SDL_Renderer *g_ren = NULL;
static TTF_Font     *g_font = NULL;
static SDL_Texture  *g_bg = NULL;   // 当前背景图

static char **g_lines = NULL;
static int   g_line_count = 0;
static int   g_pos = 0;

static void die(const char *msg) {
    FILE *f = fopen("sdmc:/switch/kzn/engine.log", "w");
    if (f) { fprintf(f, "FATAL: %s\n", msg); fclose(f); }
}

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
}

// 用 stb_image 加载 PNG 为 SDL_Texture
static SDL_Texture *load_png(const char *path) {
    int w, h, ch;
    unsigned char *pix = stbi_load(path, &w, &h, &ch, 4);
    if (!pix) return NULL;
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormatFrom(pix, w, h, 32, w * 4,
                                                           SDL_PIXELFORMAT_RGBA32);
    if (!surf) { stbi_image_free(pix); return NULL; }
    SDL_Texture *tex = SDL_CreateTextureFromSurface(g_ren, surf);
    SDL_FreeSurface(surf);
    stbi_image_free(pix);
    return tex;
}

static void set_background(const char *name) {
    if (g_bg) { SDL_DestroyTexture(g_bg); g_bg = NULL; }
    char path[256];
    snprintf(path, sizeof(path), "romfs:/bg/%s.png", name);
    g_bg = load_png(path);
}

static void render_text(const char *text) {
    SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
    SDL_RenderClear(g_ren);

    // 背景图（缩放填满 640x480）
    if (g_bg) {
        SDL_Rect dst = {0, 0, LOGIC_W, LOGIC_H};
        SDL_RenderCopy(g_ren, g_bg, NULL, &dst);
    }

    // 文字框
    SDL_Rect box = {20, LOGIC_H - 140, LOGIC_W - 40, 120};
    SDL_SetRenderDrawColor(g_ren, 20, 20, 40, 210);
    SDL_SetRenderDrawBlendMode(g_ren, SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(g_ren, &box);
    SDL_SetRenderDrawBlendMode(g_ren, SDL_BLENDMODE_NONE);

    // 逐行渲染文字
    char line[256];
    const char *p = text;
    int y = box.y + 10, row = 0;
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

// 推进到下一个 text 事件；途中处理 image/bgm/voice
static const char *advance(void) {
    while (g_pos < g_line_count) {
        char *line = g_lines[g_pos++];
        if (line[0] == 'T' && line[1] == ':') return line + 2;
        if (line[0] == 'I' && line[1] == ':') set_background(line + 2);
        // V:/B: 暂不处理
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    romfsInit();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) { die("SDL_Init"); return 1; }
    if (TTF_Init() < 0) { die("TTF_Init"); return 1; }

    g_win = SDL_CreateWindow("kzn", 0, 0, 1280, 720, SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!g_win) { die("window"); return 1; }
    g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED);
    if (!g_ren) { die("renderer"); return 1; }
    SDL_RenderSetLogicalSize(g_ren, LOGIC_W, LOGIC_H);

    g_font = TTF_OpenFont("romfs:/font.ttf", FONT_SIZE);
    if (!g_font) { die("font"); return 1; }

    load_script("romfs:/script.txt");

    const char *cur = advance();
    if (cur) render_text(cur);

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_JOYBUTTONDOWN) {
                int b = e.jbutton.button;
                if (b == 0) {
                    cur = advance();
                    if (cur) render_text(cur);
                    else running = 0;
                }
                if (b == 10) running = 0;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_SPACE || e.key.keysym.sym == SDLK_RETURN) {
                    cur = advance();
                    if (cur) render_text(cur);
                    else running = 0;
                }
                if (e.key.keysym.sym == SDLK_ESCAPE) running = 0;
            }
        }
        SDL_Delay(16);
    }

    if (g_bg) SDL_DestroyTexture(g_bg);
    TTF_CloseFont(g_font);
    SDL_DestroyRenderer(g_ren);
    SDL_DestroyWindow(g_win);
    TTF_Quit();
    SDL_Quit();
    romfsExit();
    return 0;
}
