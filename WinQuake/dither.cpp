#include <math.h>

#define MAX_X 256
#define MAX_Y 192

struct RGB { float r, g, b; };
struct SortColor { int id, value; };

class DITHER {

protected:

    struct RGB  palette[16];

    int  width, height, depth;
    char buf[256];

    struct RGB src      [MAX_X][MAX_Y];
    struct RGB source   [MAX_X][MAX_Y];
    int        dest  [2][MAX_X][MAX_Y];

    int area_bits [32][192];
    int area_attr [768];

    // Найти ближайший цвет
    int search_nearest(struct RGB needle) {

        float dist_min = 4*256*256;
        int   dist_col = 0;

        // dospal
        for (int i = 0; i < depth; i++) {

            // Сравниваемый цвет
            struct RGB cl = palette[i];

            // Дистанция между точками (r'-r)^2 + (g'-g)^2 + (b'-b)^2
            float dist = (needle.r - cl.r) * (needle.r - cl.r) +
                         (needle.g - cl.g) * (needle.g - cl.g) +
                         (needle.b - cl.b) * (needle.b - cl.b);

            if (dist < dist_min) {
                dist_min = dist;
                dist_col = i;
            }
        }

        return dist_col;
    }

public:

    DITHER() {

        depth = 8; // 8 | 16

        palette[0].r  = 0x00; palette[0].g = 0x00; palette[0].b = 0x00;  // 0 Черный
        palette[1].r  = 0x00; palette[1].g = 0x00; palette[1].b = 0xc0;  // 1 Синий
        palette[2].r  = 0xc0; palette[2].g = 0x00; palette[2].b = 0x00;  // 2 Красный
        palette[3].r  = 0xc0; palette[3].g = 0x00; palette[3].b = 0xc0;  // 3 Фиолетовый
        palette[4].r  = 0x00; palette[4].g = 0xc0; palette[4].b = 0x00;  // 4 Зеленый
        palette[5].r  = 0x00; palette[5].g = 0xc0; palette[5].b = 0xc0;  // 5 Голубой
        palette[6].r  = 0xc0; palette[6].g = 0xc0; palette[6].b = 0x00;  // 6 Желтый
        palette[7].r  = 0xc0; palette[7].g = 0xc0; palette[7].b = 0xc0;  // 7 Серый

        palette[8 ].r = 0x00; palette[8 ].g = 0x00; palette[8 ].b = 0x00; // 0
        palette[9 ].r = 0x00; palette[9 ].g = 0x00; palette[9 ].b = 0xff; // 1
        palette[10].r = 0xff; palette[10].g = 0x00; palette[10].b = 0x00; // 2
        palette[11].r = 0xff; palette[11].g = 0x00; palette[11].b = 0xff; // 3
        palette[12].r = 0x00; palette[12].g = 0xff; palette[12].b = 0x00; // 4
        palette[13].r = 0x00; palette[13].g = 0xff; palette[13].b = 0xff; // 5
        palette[14].r = 0xff; palette[14].g = 0xff; palette[14].b = 0x00; // 6
        palette[15].r = 0xff; palette[15].g = 0xff; palette[15].b = 0xff; // 7
    }

    static unsigned char color(int value)
    {
        value += 64;
        if (value > 255)
            value = 255;
        return value;
    }

    void set(unsigned char* p)
    {
        width = 256;
        height = 192;
        for (int y = 0; y < height; y++)
        for (int x = 0; x < width;  x++) {
            source[x][y].b = color(*p++);
            source[x][y].g = color(*p++);
            source[x][y].r = color(*p++);
            p++;
        }
    }

    void getsrc(unsigned char* p)
    {
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++) {
                *p++ = source[x][y].b;
                *p++ = source[x][y].g;
                *p++ = source[x][y].r;
                p++;
            }
    }

    void get(unsigned char* p)
    {
        for (int y = 0; y < height; y++)
        for (int x = 0; x < width;  x++) {
            *p++ = palette[dest[1][x][y]].b;
            *p++ = palette[dest[1][x][y]].g;
            *p++ = palette[dest[1][x][y]].r;
            p++;
        }
    }

    /*
    int  load_ppm(const char* filename) {

        FILE* fp = fopen(filename, "rb");

        if (fp == NULL) { printf("File not found\n"); exit(1); }

        fgets (buf, 256, fp); // P6
         // # Comment
        do { fgets (buf, 256, fp); } while (buf[0] == '#');
        sscanf(buf, "%d %d", &width, &height);
        fgets (buf, 256, fp); // 255

        for (int y = 0; y < height; y++)
        for (int x = 0; x < width;  x++) {

            fread(buf, 1, 3, fp);
            src[x][y].r = (unsigned char) buf[0];
            src[x][y].g = (unsigned char) buf[1];
            src[x][y].b = (unsigned char) buf[2];
        }

        fclose(fp);

        return !(width == 256 && height == 192);
    }

    void save_ppm(const char* filename) {

        FILE* fp = fopen(filename, "wb+");

        sprintf(buf, "%d %d\n", width, height);

        fputs("P6\n", fp);
        fputs("# Created by IrfanView\n", fp);
        fputs(buf, fp);
        fputs("255\n", fp);

        for (int y = 0; y < height; y++)
        for (int x = 0; x < width;  x++) {

            buf[0] = source[x][y].r;
            buf[1] = source[x][y].g;
            buf[2] = source[x][y].b;
            fwrite(buf, 1, 3, fp);
        }

        fclose(fp);
    }

    void save_scr(const char* filename) {

        unsigned char vbuf[6912];

        FILE* fp = fopen(filename, "wb+");
        for (int y = 0; y < 192; y++)
        for (int x = 0; x < 32; x++) {

            int t = 256*(y&7) + ((y&0x38)>>3)*32 + 2048*(y>>6) + x;
            vbuf[t] = area_bits[x][y];
        }

        for (int i = 0; i < 768; i++)
            vbuf[6144 + i] = area_attr[i];

        fwrite(vbuf, 1, 6912,fp);
        fclose(fp);
    }
    */

    // -----------------------------------------------------------------

    // Копировать точки во временное хранение
    void copy_source() {

        for (int y = 0; y < height; y++)
        for (int x = 0; x < width;  x++)
            source[x][y] = src[x][y];
    }

    // Приведение всех цветов к виду 0..192
    void halftoned() {

        struct RGB max = {-1, -1, -1};

        if (depth > 8) return;

        for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++) {

            if (src[x][y].r > max.r) max.r = src[x][y].r;
            if (src[x][y].r > max.g) max.g = src[x][y].g;
            if (src[x][y].r > max.b) max.b = src[x][y].b;
        }

        float smax = max.r;
        if (smax < max.g) smax = max.g;
        if (smax < max.b) smax = max.b;

        if (smax > 0) {

            float f = 192.0 / smax;
            for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++) {

                src[x][y].r *= f;
                src[x][y].g *= f;
                src[x][y].b *= f;
            }
        }
    }

    // Перевести в Ч/Б
    void grayscale() {

        for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++) {

            float f = (source[x][y].r + source[x][y].g + source[x][y].b)/3;
            source[x][y].r = f;
            source[x][y].g = f;
            source[x][y].b = f;
        }
    }

    // -----------------------------------------------------------------

    // Ближайший сосед
    void dither_nearest(int dest_id) {

        // Выполнить дизеринг
        for (int y = 0; y < height; y++)
        for (int x = 0; x < width;  x++) {

            int color_id   = search_nearest(source[x][y]);  // Поиск ближайшего цвета из палитры
            source[x][y]   = palette[color_id];             // Записать приблизительный цвет
            dest[dest_id][x][y] = color_id;                 // Полученный индекс цвета
        }
    }

    // Процедура дизеринга (0=nearest 1=floyd)
    void dither_floyd(int dest_id) {

        // Выполнить дизеринг
        for (int y = 0; y < height; y++)
        for (int x = 0; x < width;  x++) {

            struct RGB old = source[x][y];          // Старые цвета
            int color_id   = search_nearest(old);   // Поиск ближайшего цвета из палитры
            RGB ncl        = palette[color_id];     // Заменить на новый цвет (из палитры)
            source[x][y]   = ncl;                   // Записать приблизительный цвет
            dest[dest_id][x][y] = color_id;         // Полученный индекс цвета

            // Вычисляем ошибку квантования
            RGB quant;

            quant.r = (old.r - ncl.r);
            quant.g = (old.g - ncl.g);
            quant.b = (old.b - ncl.b);

            //   x 7
            // 3 5 1

            // [+1, +0] 7/16
            if (x + 1 < width) {

                source[x + 1][y].r += (quant.r * 7.0/16.0);
                source[x + 1][y].g += (quant.g * 7.0/16.0);
                source[x + 1][y].b += (quant.b * 7.0/16.0);
            }

            // [-1, +1] 3/16
            if (x - 1 >= 0 && y + 1 < height) {

                source[x-1][y+1].r += (quant.r * 3.0/16.0);
                source[x-1][y+1].g += (quant.g * 3.0/16.0);
                source[x-1][y+1].b += (quant.b * 3.0/16.0);
            }

            // [+0, +1] 5/16
            if (y + 1 < height) {

                source[x][y+1].r += (quant.r * 5.0/16.0);
                source[x][y+1].g += (quant.g * 5.0/16.0);
                source[x][y+1].b += (quant.b * 5.0/16.0);
            }

            // [+1, +1] 1/16
            if (x + 1 < width && y + 1 < height) {

                source[x+1][y+1].r += (quant.r * 1.0/16.0);
                source[x+1][y+1].g += (quant.g * 1.0/16.0);
                source[x+1][y+1].b += (quant.b * 1.0/16.0);
            }
        }
    }

    // Дизеринг с помощью матрицы Байера
    void dither_bayer(int dest_id) {

        int ditherSize = 8;

        float lookup[8][8] =
        {
            { 0, 32,  8, 40,  2, 34, 10, 42},
            {48, 16, 56, 24, 50, 18, 58, 26},
            {12, 44,  4, 36, 14, 46,  6, 38},
            {60, 28, 52, 20, 62, 30, 54, 22},
            { 3, 35, 11, 43,  1, 33,  9, 41},
            {51, 19, 59, 27, 49, 17, 57, 25},
            {15, 47,  7, 39, 13, 45,  5, 37},
            {63, 31, 55, 23, 61, 29, 53, 21},
        };

        float f = 1.5, m = 32.0;
        for (int y = 0; y < height; y++)
        for (int x = 0; x < width;  x++) {

            struct RGB new_cl = source[x][y];

            // Процедура дизеринга
            new_cl.r += (lookup[y&7][x&7])*f - m;
            new_cl.g += (lookup[y&7][x&7])*f - m;
            new_cl.b += (lookup[y&7][x&7])*f - m;

            // Поиск ближайшего цвета из палитры
            int color_id = search_nearest(new_cl);
            source[x][y] = palette[color_id];
            dest[dest_id][x][y] = color_id;
        }
    }

    // -----------------------------------------------------------------

    // Вторая версия
    void clashing_exact() {

        int i, j, k, x, y;
        struct SortColor t[2][16];

        for (y = 0; y < 24; y++)
        for (x = 0; x < 32; x++) {

            // Одни цвета для 1, другие для 0
            for (i = 0; i < 2; i++)
            for (j = 0; j < 16; j++)
                t[i][j] = {0, 0};

            // Подсчет цветов
            for (i = 0; i < 8; i++) {

                int chr = 0;
                for (j = 0; j < 8; j++) {

                    int fx = x*8 + j,
                        fy = y*8 + i,
                        cbit = dest[0][fx][fy] ? 1 : 0;

                    // Если есть тон, это 1, иначе 0
                    if (cbit) chr |= (128 >> j);

                    // Цветное изображение
                    int color_id = dest[1][fx][fy] & 15;

                    t[ cbit ][ color_id ].id = color_id;
                    t[ cbit ][ color_id ].value++;
                }

                // Записывается информация о цветах
                area_bits[x][8*y+i] = chr;
            }

            // Выстроить самые часто встречаемые цвета
            for (k = 0;   k < 2;  k++)
            for (i = 0;   i < 16; i++)
            for (j = i+1; j < 16; j++) {

                if (t[k][i].value > t[k][j].value) {
                    struct SortColor _temp  = t[k][i];
                    t[k][i] = t[k][j];
                    t[k][j] = _temp;
                }
            }

            // Анализ цветов
            int back = 0, fore = 7;
/*
            for (i = 0; i < 16; i++) { printf("%02d ", t[0][i].value); } printf("\n");
            for (i = 0; i < 16; i++) { printf("%02d ", t[1][i].value); } printf("\n");
            printf("\n----\n");
*/
            // Абсолютно однозначные цвета
            if (t[0][14].value == 0) back = t[0][15].id;
            if (t[1][14].value == 0) fore = t[1][15].id;

            // Серый цвет как 1, иначе 0
            area_attr[32*y+x] = (back&7)<<3 | (fore&7);
        }
    }

    // Конвертация изображения
    void scr2source() {

        for (int y = 0; y < 192; y++)
        for (int x = 0; x < 32; x++) {

            int at = area_attr[32*(y>>3)+x];
            int ch = area_bits[x][y];
            int hb = at & 0x40? 8 : 0;

            RGB fr = palette[hb | (at & 7)];
            RGB bk = palette[hb | ((at>>3) & 7)];

            for (int i = 0; i < 8; i++) {
                source[8*x+i][y] = (ch << i) & 128 ? fr : bk;
            }
        }
    }

    // Создание клешинга 8x8
    void clashing() {

        for (int y = 0; y < 24; y++) {
            for (int x = 0; x < 32; x++) {

                area_attr[32*y+x] = 0;

                // Поиск наиболее "популярных" цветов
                int color_table[16];
                int max1    = -1, max2    = -1,
                    max1_id = 0,  max2_id = 0;

                // Очистка цветовой таблицы
                for (int i = 0; i < 16; i++)
                    color_table[i] = 0;

                // Подсчет цветов
                for (int i = 0; i < 8; i++)
                for (int j = 0; j < 8; j++)
                    color_table[ dest[1][x*8+j][y*8+i] & 15 ]++;

                // Самый популярный цвет
                for (int i = 0; i < 16; i++)
                    if (color_table[i] > max1) {
                        max1 = color_table[i];
                        max1_id = i;
                    }

                // Второй по популярности
                for (int i = 0; i < 16; i++) {
                    if (i == max1_id) continue;
                    if (color_table[i] > max2) {
                        max2 = color_table[i];
                        max2_id = i;
                    }
                }

                // Подтянуть менее популярный цвет до яркости популярного
                if ((max1_id&8) != (max2_id&8)) max2_id = (max2_id & 7) | (max1_id&8);

                //max1_id |= 8;
                //max2_id |= 8;

                struct RGB attr = palette[max1_id];
                struct RGB back = palette[max2_id];

                // Обновление новых атрибутов
                for (int i = 0; i < 8; i++) {

                    area_bits[x][8*y+i] = 0;

                    for (int j = 0; j < 8; j++) {

                        struct RGB cl = source[8*x+j][8*y+i];

                        // Насколько цвет близок к back
                        float dist1 = (cl.r-back.r)*(cl.r-back.r) +
                                      (cl.g-back.g)*(cl.g-back.g) +
                                      (cl.b-back.b)*(cl.b-back.b);

                        // Насколько цвет близок к attr
                        float dist2 = (cl.r-attr.r)*(cl.r-attr.r) +
                                      (cl.g-attr.g)*(cl.g-attr.g) +
                                      (cl.b-attr.b)*(cl.b-attr.b);

                        // Что ближе? dist1<dist2 --> бит 0, иначе 1
                        source[8*x+j][8*y+i] = dist1 < dist2 ? back : attr;

                        // Запись битовой маски
                        if (dist1 >= dist2) area_bits[x][8*y+i] |= (128 >> j);
                    }
                }

                area_attr[y*32+x] = (max1_id & 8 ? 0x40 : 0x00) | (max1_id&7) | ((max2_id&7)<<3); // 0x40
            }
        }
    }
};

extern "C" void Dither(unsigned char* p, int color);
void Dither(unsigned char* p, int color)
{
    static DITHER d;
    d.set(p);
    if (!color)
        d.grayscale();
    d.dither_nearest(1);
    if (!color)
        d.get(p);
    else {
        d.clashing();
        d.scr2source();
        d.getsrc(p);
    }
}
