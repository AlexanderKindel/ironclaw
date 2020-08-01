#include <stdbool.h>
#include <stdint.h>
#include <windows.h>
#include <windowsx.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof(arr[0]))

int32_t min32(int32_t a, int32_t b)
{
    if (a < b)
    {
        return a;
    }
    return b;
}

int32_t max32(int32_t a, int32_t b)
{
    if (a > b)
    {
        return a;
    }
    return b;
}

typedef struct Stack
{
    void*start;
    void*end;
    void*cursor;
    void*cursor_max;
} Stack;

size_t g_page_size;
Stack g_stack;

void*align_cursor(Stack*output_stack, size_t alignment)
{
    output_stack->cursor =
        (void*)(((uintptr_t)output_stack->cursor + alignment - 1) & -(uintptr_t)alignment);
    return output_stack->cursor;
}

void allocate_unaligned_stack_slot(Stack*output_stack, size_t slot_size)
{
    output_stack->cursor = (uint8_t*)output_stack->cursor + slot_size;
    while (output_stack->cursor > output_stack->cursor_max)
    {
        VirtualAlloc(output_stack->cursor_max, g_page_size, MEM_COMMIT, PAGE_READWRITE);
        output_stack->cursor_max = (uint8_t*)output_stack->cursor_max + g_page_size;
    }
}

void*allocate_stack_slot(Stack*output_stack, size_t slot_size, size_t alignment)
{
    void*slot = align_cursor(output_stack, alignment);
    allocate_unaligned_stack_slot(output_stack, slot_size);
    return slot;
}

typedef struct Rect
{
    int32_t min_x;
    int32_t min_y;
    int32_t width;
    int32_t height;
} Rect;

typedef union Color
{
    struct
    {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
        uint8_t alpha;
    };
    uint32_t value;
} Color;

BITMAPINFO g_pixel_buffer_info;
Color*g_pixels = 0;
Rect g_window_rect = { 0, 0 };

uint32_t g_line_thickness;
uint32_t g_table_row_height;

Color g_black = { .value = 0xff000000 };
Color g_dark_gray = { .value = 0xffb0b0b0 };
Color g_light_gray = { .value = 0xffd0d0d0 };
Color g_white = { .value = 0xffffffff };

void draw_filled_rectangle(Rect*clip_rect, int32_t min_x, int32_t min_y, uint32_t width,
    uint32_t height, Color color)
{
    Color*top_left_in_window_buffer = g_pixels + min_y * g_window_rect.width + min_x;
    int32_t max_x = min32(width, clip_rect->min_x + clip_rect->width - min_x);
    int32_t max_y = min32(height, clip_rect->min_y + clip_rect->height - min_y);
    for (int32_t y = max32(0, clip_rect->min_y - min_y); y < max_y; ++y)
    {
        Color*row = top_left_in_window_buffer + y * g_window_rect.width;
        for (int32_t x = max32(0, clip_rect->min_x - min_x); x < max_x; ++x)
        {
            row[x].value = color.value;
        }
    }
}

void draw_rectangle_outline(int32_t min_x, int32_t min_y, uint32_t width, uint32_t height,
    Color color)
{
    draw_filled_rectangle(&g_window_rect, min_x, min_y, width, g_line_thickness, color);
    draw_filled_rectangle(&g_window_rect, min_x + g_line_thickness, min_y + height, width,
        g_line_thickness, color);
    draw_filled_rectangle(&g_window_rect, min_x, min_y + g_line_thickness, g_line_thickness,
        height, color);
    draw_filled_rectangle(&g_window_rect, min_x + width, min_y, g_line_thickness, height, color);
}

typedef struct Grid
{
    size_t column_count;
    size_t row_count;
    uint32_t*column_offsets;
} Grid;

void draw_grid_dividers(Grid*grid, Rect*clip_rect, int32_t min_x, int32_t min_y)
{
    int32_t cell_min_y = min_y + g_line_thickness;
    uint32_t height = g_table_row_height * grid->row_count - g_line_thickness;
    for (size_t column_index = 1; column_index < grid->column_count; ++column_index)
    {
        draw_filled_rectangle(clip_rect, min_x + grid->column_offsets[column_index], cell_min_y,
            g_line_thickness, height, g_black);
    }
    int32_t cell_min_x = min_x + g_line_thickness;
    for (size_t row_index = 1; row_index < grid->row_count; ++row_index)
    {
        min_y += g_table_row_height;
        draw_filled_rectangle(clip_rect, cell_min_x, min_y,
            grid->column_offsets[grid->column_count], g_line_thickness, g_black);
    }
}

void draw_grid(Grid*grid, uint32_t min_x, uint32_t min_y)
{
    draw_rectangle_outline(min_x, min_y, grid->column_offsets[grid->column_count],
        g_table_row_height * grid->row_count, g_black);
    draw_grid_dividers(grid, &g_window_rect, min_x, min_y);
}

typedef struct Rasterization
{
    FT_Bitmap bitmap;
    FT_Int left_bearing;
    FT_Int top_bearing;
    FT_Pos advance;
} Rasterization;

#define FIRST_RASTERIZED_GLYPH ' '
#define LAST_RASTERIZED_GLYPH '~'

FT_Library g_freetype_library;
FT_Face g_face;

Rasterization g_glyph_rasterizations[1 + LAST_RASTERIZED_GLYPH - FIRST_RASTERIZED_GLYPH];
Rasterization g_right_arrowhead_rasterization;
Rasterization g_left_arrowhead_rasterization;

FT_F26Dot6 g_message_font_height;
uint32_t g_text_padding;

uint32_t round26_6to_pixel(uint32_t subpixel)
{
    return (subpixel + 32) >> 6;
}

Rasterization*get_rasterization(char codepoint)
{
    return g_glyph_rasterizations + codepoint - FIRST_RASTERIZED_GLYPH;
}

void rasterize_glyph(FT_ULong codepoint, Rasterization*rasterization_slot)
{
    FT_Load_Glyph(g_face, FT_Get_Char_Index(g_face, codepoint), FT_LOAD_DEFAULT);
    FT_Render_Glyph(g_face->glyph, FT_RENDER_MODE_NORMAL);
    rasterization_slot->bitmap = g_face->glyph->bitmap;
    rasterization_slot->bitmap.buffer = g_stack.cursor;
    size_t rasterization_size = g_face->glyph->bitmap.rows * g_face->glyph->bitmap.pitch;
    allocate_unaligned_stack_slot(&g_stack, rasterization_size);
    memcpy(rasterization_slot->bitmap.buffer, g_face->glyph->bitmap.buffer, rasterization_size);
    rasterization_slot->left_bearing = g_face->glyph->bitmap_left;
    rasterization_slot->top_bearing = g_face->glyph->bitmap_top;
    rasterization_slot->advance = g_face->glyph->metrics.horiAdvance;
}

void draw_rasterization(Rect*clip_rect, Rasterization*rasterization, int32_t min_x, int32_t min_y,
    Color color)
{
    int32_t max_x_in_bitmap = min32(rasterization->bitmap.width,
        clip_rect->min_x + clip_rect->width - min_x);
    int32_t max_y_in_bitmap =
        min32(rasterization->bitmap.rows, clip_rect->min_y + clip_rect->height - min_y);
    Color*bitmap_top_left_in_window_buffer = g_pixels + min_y * g_window_rect.width + min_x;
    for (int32_t bitmap_y = max32(0, clip_rect->min_y - min_y); bitmap_y < max_y_in_bitmap;
        ++bitmap_y)
    {
        Color*bitmap_row_left_end_in_window_buffer =
            bitmap_top_left_in_window_buffer + bitmap_y * g_window_rect.width;
        for (LONG bitmap_x = max32(0, clip_rect->min_x - min_x); bitmap_x < max_x_in_bitmap;
            ++bitmap_x)
        {
            Color*pixel = bitmap_row_left_end_in_window_buffer + bitmap_x;
            uint32_t alpha =
                rasterization->bitmap.buffer[bitmap_y * rasterization->bitmap.pitch + bitmap_x];
            uint32_t lerp_term = (255 - alpha) * pixel->alpha;
            pixel->red = (color.red * alpha) / 255 + (lerp_term * pixel->red) / 65025;
            pixel->green = (color.green * alpha) / 255 + (lerp_term * pixel->green) / 65025;
            pixel->blue = (color.blue * alpha) / 255 + (lerp_term * pixel->blue) / 65025;
            pixel->alpha = alpha + lerp_term / 255;
        }
    }
}

FT_Pos draw_string(char*string, Rect*clip_rect, int32_t origin_x, int32_t origin_y)
{
    FT_Pos advance = origin_x << 6;
    FT_UInt previous_glyph_index = FT_Get_Char_Index(g_face, string[0]);
    FT_Vector kerning_with_previous_glyph = { 0, 0 };
    for (char*character = string; *character; ++character)
    {
        advance += kerning_with_previous_glyph.x;
        Rasterization*rasterization = get_rasterization(*character);
        draw_rasterization(clip_rect, rasterization,
            round26_6to_pixel(advance) + rasterization->left_bearing,
            origin_y - rasterization->top_bearing, g_black);
        advance += g_glyph_rasterizations[*character - FIRST_RASTERIZED_GLYPH].advance;
        FT_UInt glyph_index = FT_Get_Char_Index(g_face, character[1]);
        FT_Get_Kerning(g_face, previous_glyph_index, glyph_index, FT_KERNING_DEFAULT,
            &kerning_with_previous_glyph);
    }
    return advance;
}

uint32_t get_string_width(char*string)
{
    FT_Pos out = 0;
    FT_UInt previous_glyph_index = FT_Get_Char_Index(g_face, string[0]);
    FT_Vector kerning_with_previous_glyph = { 0, 0 };
    for (char*character = string; *character; ++character)
    {
        out += kerning_with_previous_glyph.x + get_rasterization(*character)->advance;
        FT_Get_Kerning(g_face, previous_glyph_index, FT_Get_Char_Index(g_face, character[1]),
            FT_KERNING_DEFAULT, &kerning_with_previous_glyph);
    }
    return round26_6to_pixel(out);
}

void draw_horizontally_centered_string(char*string, Rect*clip_rect, int32_t cell_min_x,
    int32_t text_origin_y, uint32_t cell_width)
{
    draw_string(string, clip_rect, cell_min_x + (cell_width - get_string_width(string)) / 2,
        text_origin_y);
}

void draw_uint8(Rect*clip_rect, uint8_t value, int32_t origin_x, int32_t origin_y)
{
    char buffer[4];
    char*string;
    if (value)
    {
        string = buffer + 3;
        *string = 0;
        while (value)
        {
            --string;
            *string = value % 10 + '0';
            value /= 10;
        }
    }
    else
    {
        string = buffer;
        string[0] = '0';
        string[1] = 0;
    }
    draw_string(string, clip_rect, origin_x, origin_y);
}

#define PARENT_CONTROL_ID_MASK 0b11100000
#define CHILD_CONTROL_ID_MASK 0b11111
#define NULL_CONTROL_ID 0b11111111
#define SKILL_TABLE_ID 0b100000
#define SKILL_TABLE_SCROLL_BAR_THUMB_ID 0b1000000
#define SPEND_DIE_BUTTON_ID 0b1100000
#define UNSPEND_DIE_BUTTON_ID 0b10000000
#define UNSPENT_DICE_TABLE_ID 0b10100000
#define TABS_ID 0b11000000

uint32_t g_clicked_control_id = 0;
int32_t g_cursor_x;
int32_t g_cursor_y;
bool g_left_mouse_button_is_down = false;
bool g_left_mouse_button_changed_state = false;

bool do_button_action(Rect*clip_rect, uint32_t control_id, int32_t min_x, int32_t min_y,
    uint32_t width, uint32_t height)
{
    int32_t cell_min_x = min_x + g_line_thickness;
    int32_t cell_min_y = min_y + g_line_thickness;
    if (g_cursor_x >= cell_min_x && g_cursor_x < min_x + width && g_cursor_y >= cell_min_y &&
        g_cursor_y < min_y + height)
    {
        if (g_left_mouse_button_is_down)
        {
            if (g_left_mouse_button_changed_state)
            {
                g_clicked_control_id = control_id;
            }
            if (g_clicked_control_id == control_id)
            {
                draw_filled_rectangle(clip_rect, cell_min_x, cell_min_y, width - g_line_thickness,
                    height - g_line_thickness, g_white);
            }
        }
        else
        {
            draw_filled_rectangle(clip_rect, cell_min_x, cell_min_y, width - g_line_thickness,
                height - g_line_thickness, g_light_gray);
            if (g_left_mouse_button_changed_state && g_clicked_control_id == control_id)
            {
                g_clicked_control_id = NULL_CONTROL_ID;
                return true;
            }
        }
    }
    else if (g_clicked_control_id == control_id)
    {
        if (g_left_mouse_button_is_down)
        {
            uint32_t interior_dimension = g_table_row_height - g_line_thickness;
            draw_filled_rectangle(clip_rect, cell_min_x, cell_min_y, width - g_line_thickness,
                height - g_line_thickness, g_light_gray);
        }
        else if (g_left_mouse_button_changed_state)
        {
            g_clicked_control_id = NULL_CONTROL_ID;
        }
    }
    return false;
}

void select_table_row(Rect*clip_rect, size_t*selected_row_index, size_t table_row_count,
    int32_t table_min_x, int32_t table_min_y, uint32_t table_width, uint32_t table_id)
{
    if (g_cursor_y >= table_min_y)
    {
        size_t row_index = (g_cursor_y - table_min_y) / g_table_row_height;
        if (row_index < table_row_count &&
            do_button_action(clip_rect, table_id | row_index, table_min_x,
                table_min_y + row_index * g_table_row_height, table_width, g_table_row_height))
        {
            *selected_row_index = row_index;
        }
        if ((g_clicked_control_id & PARENT_CONTROL_ID_MASK) == table_id)
        {
            do_button_action(clip_rect, g_clicked_control_id, table_min_x,
                table_min_y + (g_clicked_control_id & CHILD_CONTROL_ID_MASK) * g_table_row_height,
                table_width, g_table_row_height);
        }
    }
}

char*g_die_denominations[] = { "D4", "D6", "D8", "D10", "D12" };

typedef struct Skill
{
    char*name;
    uint8_t dice[ARRAY_COUNT(g_die_denominations)];
    uint8_t mark_count;
} Skill;

Skill g_skills[] = { { "Academics" }, { "Brawling" }, { "Climbing" }, { "Craft" }, { "Deceit" },
{ "Digging" }, { "Dodge" }, { "Endurance" }, { "Gossip" }, { "Inquiry" }, { "Jumping" },
{ "Leadership" }, { "Mle Combat" }, { "Negotiation" }, { "Observation" }, { "Presence" },
{ "Ranged Combat" }, { "Riding" }, { "Searching" }, { "Stealth" }, { "Supernatural" },
{ "Swimming" }, { "Tactics" }, { "Throwing" }, { "Vehicles" }, { "Weather Sense" } };
Grid g_skill_table = { 7, ARRAY_COUNT(g_skills), (uint32_t[8]) { 0 } };
Grid g_skill_dice_header =
{ ARRAY_COUNT(g_die_denominations), 1, (uint32_t[ARRAY_COUNT(g_die_denominations) + 1]) { 0 } };
size_t g_selected_skill_index = ARRAY_COUNT(g_skills);

uint32_t g_skill_table_viewport_offset = 0;
uint32_t g_skill_previous_table_viewport_offset;
int32_t g_previous_skill_table_scroll_bar_cursor_y;

uint8_t g_unspent_dice[] = { 1, 3, 2 };
Grid g_unspent_dice_table =
{ ARRAY_COUNT(g_unspent_dice), 2, (uint32_t[ARRAY_COUNT(g_unspent_dice) + 1]) { 0 } };
size_t g_selected_die_denomination_index = ARRAY_COUNT(g_unspent_dice);

char*g_tab_names[] = { "Allocate Trait Dice", "Allocate Marks" };
uint32_t g_tab_width;
size_t g_selected_tab_index = 0;

void scale_window_contents(HWND window_handle, WORD dpi)
{
    HFONT win_font = CreateFont(0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 0, "Ariel");
    HDC device_context = GetDC(window_handle);
    SelectFont(device_context, win_font);
    DWORD font_file_size = GetFontData(device_context, 0, 0, 0, 0);
    void*font_data = g_stack.cursor;
    allocate_unaligned_stack_slot(&g_stack, font_file_size);
    GetFontData(device_context, 0, 0, font_data, font_file_size);
    DeleteObject(win_font);
    ReleaseDC(window_handle, device_context);
    FT_New_Memory_Face(g_freetype_library, font_data, font_file_size, 0, &g_face);
    FT_Set_Char_Size(g_face, 0, g_message_font_height, 0, dpi);
    for (FT_ULong codepoint = FIRST_RASTERIZED_GLYPH; codepoint <= LAST_RASTERIZED_GLYPH;
        ++codepoint)
    {
        rasterize_glyph(codepoint, get_rasterization(codepoint));
    }
    rasterize_glyph(0x25ba, &g_right_arrowhead_rasterization);
    rasterize_glyph(0x25c4, &g_left_arrowhead_rasterization);
    g_line_thickness = get_rasterization('|')->bitmap.width;
    g_table_row_height =
        round26_6to_pixel(g_face->size->metrics.ascender - 2 * g_face->size->metrics.descender);
    g_text_padding = (g_table_row_height - get_rasterization('M')->bitmap.rows) / 2;
    g_table_row_height += g_line_thickness;
    g_skill_table.column_offsets[1] = get_string_width("Skills");
    for (size_t i = 0; i < ARRAY_COUNT(g_skills); ++i)
    {
        uint32_t string_width = get_string_width(g_skills[i].name);
        if (string_width > g_skill_table.column_offsets[1])
        {
            g_skill_table.column_offsets[1] = string_width;
        }
    }
    g_skill_table.column_offsets[1] += g_line_thickness + 2 * g_text_padding;
    g_skill_table.column_offsets[2] = g_skill_table.column_offsets[1] + get_string_width("Marks") +
        2 * g_text_padding + g_line_thickness;
    uint32_t die_column_width = g_line_thickness + get_string_width("D12") + 2 * g_text_padding;
    for (size_t i = 1; i <= g_skill_dice_header.column_count; ++i)
    {
        g_skill_dice_header.column_offsets[i] =
            g_skill_dice_header.column_offsets[i - 1] + die_column_width;
        g_skill_table.column_offsets[i + 2] =
            g_skill_table.column_offsets[2] + g_skill_dice_header.column_offsets[i];
    }
    for (size_t i = 0; i <= g_unspent_dice_table.column_count; ++i)
    {
        g_unspent_dice_table.column_offsets[i] = i * die_column_width;
    }
    g_tab_width = g_line_thickness + 2 * g_text_padding +
        max32(get_string_width(g_tab_names[0]), get_string_width(g_tab_names[1]));
}

LRESULT CALLBACK create_character_proc(HWND window_handle, UINT message, WPARAM w_param,
    LPARAM l_param)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_DPICHANGED:
        g_stack.cursor = g_stack.start;
        FT_Done_Face(g_face);
        scale_window_contents(window_handle, HIWORD(w_param));
        RECT*client_rect = (RECT*)l_param;
        SetWindowPos(window_handle, 0, client_rect->left, client_rect->top,
            client_rect->right - client_rect->left, client_rect->bottom - client_rect->top,
            SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
        return 0;
    case WM_LBUTTONDOWN:
        SetCapture(window_handle);
        g_left_mouse_button_is_down = true;
        g_left_mouse_button_changed_state = true;
        return 0;
    case WM_LBUTTONUP:
        ReleaseCapture();
        g_left_mouse_button_is_down = false;
        g_left_mouse_button_changed_state = true;
        return 0;
    case WM_MOUSEMOVE:
        g_cursor_x = GET_X_LPARAM(l_param);
        g_cursor_y = GET_Y_LPARAM(l_param);
        return 0;
    case WM_SIZE:
    {
        RECT client_rect;
        GetClientRect(window_handle, &client_rect);
        g_window_rect.width = client_rect.right - client_rect.left;
        g_window_rect.height = client_rect.bottom - client_rect.top;
        g_pixel_buffer_info.bmiHeader.biWidth = g_window_rect.width;
        g_pixel_buffer_info.bmiHeader.biHeight = -(int32_t)g_window_rect.height;
        VirtualFree(g_pixels, 0, MEM_RELEASE);
        g_pixels = VirtualAlloc(0, sizeof(Color) * g_window_rect.width * g_window_rect.height,
            MEM_COMMIT, PAGE_READWRITE);
        return 0;
    }
    }
    return DefWindowProc(window_handle, message, w_param, l_param);
}

HWND init(HINSTANCE instance_handle)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    if (system_info.dwAllocationGranularity > system_info.dwPageSize)
    {
        g_page_size = system_info.dwAllocationGranularity;
    }
    else
    {
        g_page_size = system_info.dwPageSize;
    }
    g_stack.start = VirtualAlloc(0, UINT32_MAX, MEM_RESERVE, PAGE_READWRITE);
    g_stack.end = (uint8_t*)g_stack.start + UINT32_MAX;
    g_stack.cursor = g_stack.start;
    g_stack.cursor_max = g_stack.start;
    g_pixel_buffer_info.bmiHeader.biSize = sizeof(g_pixel_buffer_info.bmiHeader);
    g_pixel_buffer_info.bmiHeader.biPlanes = 1;
    g_pixel_buffer_info.bmiHeader.biBitCount = 8 * sizeof(Color);
    g_pixel_buffer_info.bmiHeader.biCompression = BI_RGB;
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = create_character_proc;
    wc.hInstance = instance_handle;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.lpszClassName = "Create Character";
    RegisterClassEx(&wc);
    HWND window_handle = CreateWindow(wc.lpszClassName, "Create Character", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance_handle, 0);
    NONCLIENTMETRICSW non_client_metrics;
    non_client_metrics.cbSize = sizeof(NONCLIENTMETRICSW);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, non_client_metrics.cbSize, &non_client_metrics,
        0);
    WORD dpi = GetDpiForWindow(window_handle);
    g_message_font_height = ((-non_client_metrics.lfMessageFont.lfHeight * 72) << 6) / dpi;
    FT_Init_FreeType(&g_freetype_library);
    scale_window_contents(window_handle, dpi);
    POINT cursor_position;
    GetCursorPos(&cursor_position);
    g_cursor_x = cursor_position.x;
    g_cursor_y = cursor_position.y;
    ShowWindow(window_handle, SW_MAXIMIZE);
    return window_handle;
}

int WINAPI wWinMain(HINSTANCE instance_handle, HINSTANCE previous_instance_handle,
    PWSTR command_line, int show)
{
    HWND window_handle = init(instance_handle);
    HDC device_context = GetDC(window_handle);
    MSG msg;
    while (GetMessage(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        for (size_t i = 0; i < g_window_rect.width * g_window_rect.height; ++i)
        {
            g_pixels[i].value = g_dark_gray.value;
        }
        uint32_t cell_height = g_table_row_height - g_line_thickness;
        RECT client_rect;
        GetClientRect(window_handle, &client_rect);
        int32_t skill_table_min_y = cell_height + g_table_row_height;
        Rect skill_table_viewport;
        skill_table_viewport.min_x = g_table_row_height;
        skill_table_viewport.min_y = skill_table_min_y + g_line_thickness;
        skill_table_viewport.width =
            g_skill_table.column_offsets[g_skill_table.column_count] - g_line_thickness;
        skill_table_viewport.height =
            client_rect.bottom - (skill_table_viewport.min_y + g_table_row_height);
        uint32_t skill_table_cells_height =
            g_table_row_height * g_skill_table.row_count - g_line_thickness;
        if (skill_table_viewport.height < skill_table_cells_height)
        {
            draw_filled_rectangle(&g_window_rect, 0, skill_table_viewport.min_y - g_line_thickness,
                cell_height, skill_table_viewport.height + 2 * g_line_thickness, g_light_gray);
            uint32_t thumb_width = cell_height - 2 * g_line_thickness;
            uint32_t thumb_height = (skill_table_viewport.height * skill_table_viewport.height) /
                skill_table_cells_height;
            uint32_t max_thumb_offset = skill_table_viewport.height - thumb_height;
            uint32_t max_viewport_offset = skill_table_cells_height - skill_table_viewport.height;
            int32_t thumb_min_y = skill_table_viewport.min_y;
            if (g_left_mouse_button_is_down &&
                g_clicked_control_id == SKILL_TABLE_SCROLL_BAR_THUMB_ID)
            {
                int32_t thumb_offset =
                    (max_thumb_offset * g_skill_previous_table_viewport_offset +
                        max_viewport_offset / 2) / max_viewport_offset +
                    g_cursor_y - g_previous_skill_table_scroll_bar_cursor_y;
                if (thumb_offset > 0)
                {
                    if (thumb_offset > max_thumb_offset)
                    {
                        thumb_offset = max_thumb_offset;
                    }
                }
                else
                {
                    thumb_offset = 0;
                }
                thumb_min_y += thumb_offset;
                g_skill_table_viewport_offset =
                    (max_viewport_offset * thumb_offset + max_thumb_offset / 2) / max_thumb_offset;
                g_skill_previous_table_viewport_offset = g_skill_table_viewport_offset;
                g_previous_skill_table_scroll_bar_cursor_y = g_cursor_y;
            }
            else
            {
                g_skill_table_viewport_offset =
                    min32(g_skill_table_viewport_offset, max_viewport_offset);
                thumb_min_y += (max_thumb_offset * g_skill_table_viewport_offset +
                    max_viewport_offset / 2) / max_viewport_offset;
                if (g_left_mouse_button_is_down && g_left_mouse_button_changed_state &&
                    g_cursor_x >= g_line_thickness && g_cursor_x < g_cursor_x + thumb_width &&
                    g_cursor_y >= thumb_min_y && g_cursor_y < thumb_min_y + thumb_height)
                {
                    g_clicked_control_id = SKILL_TABLE_SCROLL_BAR_THUMB_ID;
                    g_skill_previous_table_viewport_offset = g_skill_table_viewport_offset;
                    g_previous_skill_table_scroll_bar_cursor_y = g_cursor_y;
                }
            }
            draw_filled_rectangle(&g_window_rect, g_line_thickness, thumb_min_y, thumb_width,
                thumb_height, g_dark_gray);
        }
        else
        {
            g_skill_table_viewport_offset = 0;
            skill_table_viewport.height = skill_table_cells_height;
        }
        uint32_t die_column_width =
            g_skill_table.column_offsets[3] - g_skill_table.column_offsets[2];
        uint32_t die_cell_width = die_column_width - g_line_thickness;
        uint32_t dice_cells_width =
            ARRAY_COUNT(g_die_denominations) * die_column_width - g_line_thickness;
        int32_t skill_table_content_min_y = skill_table_min_y - g_skill_table_viewport_offset;
        select_table_row(&skill_table_viewport, &g_selected_skill_index, ARRAY_COUNT(g_skills),
            cell_height, skill_table_content_min_y,
            g_skill_table.column_offsets[g_skill_table.column_count], SKILL_TABLE_ID);
        if (g_selected_skill_index < ARRAY_COUNT(g_skills))
        {
            draw_filled_rectangle(&skill_table_viewport,
                g_table_row_height + g_skill_table.column_offsets[2],
                skill_table_content_min_y + g_selected_skill_index * g_table_row_height,
                dice_cells_width, g_table_row_height, g_white);
        }
        draw_grid_dividers(&g_skill_table, &skill_table_viewport, cell_height,
            skill_table_content_min_y);
        draw_rectangle_outline(cell_height, skill_table_min_y,
            g_skill_table.column_offsets[g_skill_table.column_count],
            skill_table_viewport.height + g_line_thickness, g_black);
        draw_horizontally_centered_string("Skills", &g_window_rect, g_table_row_height,
            skill_table_min_y - g_text_padding, g_skill_table.column_offsets[1] - g_line_thickness);
        draw_string("Marks", &g_window_rect,
            g_table_row_height + g_skill_table.column_offsets[1] + g_text_padding,
            skill_table_min_y - g_text_padding);
        draw_grid(&g_skill_dice_header, cell_height + g_skill_table.column_offsets[2], cell_height);
        for (size_t i = 0; i < ARRAY_COUNT(g_die_denominations); ++i)
        {
            draw_horizontally_centered_string(g_die_denominations[i], &g_window_rect,
                g_table_row_height + g_skill_table.column_offsets[i + 2],
                skill_table_min_y - g_text_padding, die_cell_width);
        }
        draw_horizontally_centered_string("Trait Dice", &g_window_rect,
            g_table_row_height + g_skill_table.column_offsets[2], cell_height - g_text_padding,
            dice_cells_width);
        int32_t row_text_y = skill_table_content_min_y - g_text_padding;
        for (size_t i = 0; i < ARRAY_COUNT(g_skills); ++i)
        {
            row_text_y += g_table_row_height;
            Skill*skill = g_skills + i;
            draw_string(skill->name, &skill_table_viewport, g_table_row_height + g_text_padding,
                row_text_y);
            draw_uint8(&skill_table_viewport, skill->mark_count,
                g_table_row_height + g_skill_table.column_offsets[1] + g_text_padding, row_text_y);
            int32_t die_text_x =
                g_table_row_height + g_skill_table.column_offsets[2] + g_text_padding;
            for (size_t die_index = 0; die_index < ARRAY_COUNT(g_die_denominations); ++die_index)
            {
                draw_uint8(&skill_table_viewport, skill->dice[die_index], die_text_x, row_text_y);
                die_text_x += die_column_width;
            }
        }
        int32_t buttons_min_x = g_table_row_height + cell_height +
            g_skill_table.column_offsets[g_skill_table.column_count];
        uint32_t unspent_dice_table_height = 2 * g_table_row_height;
        int32_t unspent_dice_table_min_x = buttons_min_x + unspent_dice_table_height;
        int32_t unspent_dice_cell_min_x = unspent_dice_table_min_x + g_line_thickness;
        if (g_cursor_x >= unspent_dice_cell_min_x)
        {
            size_t die_index = (g_cursor_x - unspent_dice_cell_min_x) / die_column_width;
            if (die_index < 3)
            {
                if (do_button_action(&g_window_rect, UNSPENT_DICE_TABLE_ID | die_index,
                    unspent_dice_table_min_x + die_index * die_column_width,
                    skill_table_content_min_y, die_column_width, unspent_dice_table_height))
                {
                    g_selected_die_denomination_index = die_index;
                }
            }
            if ((g_clicked_control_id & PARENT_CONTROL_ID_MASK) == UNSPENT_DICE_TABLE_ID)
            {
                do_button_action(&g_window_rect, g_clicked_control_id,
                    unspent_dice_table_min_x +
                    (g_clicked_control_id & CHILD_CONTROL_ID_MASK) * die_column_width,
                    skill_table_content_min_y, die_column_width, unspent_dice_table_height);
            }
        }
        if (g_selected_die_denomination_index < 3)
        {
            draw_filled_rectangle(&g_window_rect, unspent_dice_cell_min_x +
                g_selected_die_denomination_index * die_column_width,
                skill_table_viewport.min_y + g_table_row_height, die_cell_width, cell_height,
                g_white);
        }
        draw_grid(&g_unspent_dice_table, unspent_dice_table_min_x, skill_table_min_y);
        uint32_t unspent_dice_cells_width =
            g_unspent_dice_table.column_offsets[g_unspent_dice_table.column_count] -
            g_line_thickness;
        draw_horizontally_centered_string("Unallocated Dice", &g_window_rect,
            unspent_dice_cell_min_x, skill_table_min_y - g_text_padding, unspent_dice_cells_width);
        int32_t cell_x = unspent_dice_cell_min_x;
        int32_t unspend_die_button_min_y = skill_table_min_y + g_table_row_height;
        row_text_y = unspend_die_button_min_y - g_text_padding;
        for (size_t i = 0; i < g_unspent_dice_table.column_count; ++i)
        {
            draw_horizontally_centered_string(g_die_denominations[i], &g_window_rect, cell_x,
                row_text_y, die_cell_width);
            draw_uint8(&g_window_rect, g_unspent_dice[i], cell_x + g_text_padding,
                row_text_y + g_table_row_height);
            cell_x += die_column_width;
        }
        Color spend_die_border_color = g_light_gray;
        Color unspend_die_border_color = g_light_gray;
        if (g_selected_skill_index < ARRAY_COUNT(g_skills) && g_selected_die_denomination_index < 3)
        {
            uint8_t*selected_skill_dice =
                g_skills[g_selected_skill_index].dice + g_selected_die_denomination_index;
            if (g_unspent_dice[g_selected_die_denomination_index])
            {
                if (do_button_action(&g_window_rect, SPEND_DIE_BUTTON_ID, buttons_min_x,
                    skill_table_min_y, g_table_row_height, g_table_row_height))
                {
                    ++*selected_skill_dice;
                    --g_unspent_dice[g_selected_die_denomination_index];
                }
                spend_die_border_color = g_black;
            }
            if (*selected_skill_dice)
            {
                if (do_button_action(&g_window_rect, UNSPEND_DIE_BUTTON_ID, buttons_min_x,
                    unspend_die_button_min_y, g_table_row_height, g_table_row_height))
                {
                    --*selected_skill_dice;
                    ++g_unspent_dice[g_selected_die_denomination_index];
                }
                unspend_die_border_color = g_black;
            }
        }
        if (spend_die_border_color.value == g_black.value)
        {
            draw_rectangle_outline(buttons_min_x, unspend_die_button_min_y, g_table_row_height,
                g_table_row_height, unspend_die_border_color);
            draw_rectangle_outline(buttons_min_x, skill_table_min_y, g_table_row_height,
                g_table_row_height, spend_die_border_color);
        }
        else
        {
            draw_rectangle_outline(buttons_min_x, skill_table_min_y, g_table_row_height,
                g_table_row_height, spend_die_border_color);
            draw_rectangle_outline(buttons_min_x, unspend_die_button_min_y, g_table_row_height,
                g_table_row_height, unspend_die_border_color);
        }
        draw_rasterization(&g_window_rect, &g_right_arrowhead_rasterization,
            buttons_min_x + (g_table_row_height + g_line_thickness -
                g_right_arrowhead_rasterization.bitmap.width) / 2,
            unspend_die_button_min_y + (g_table_row_height + g_line_thickness -
                g_right_arrowhead_rasterization.bitmap.rows) / 2, unspend_die_border_color);
        draw_rasterization(&g_window_rect, &g_left_arrowhead_rasterization,
            buttons_min_x + (g_table_row_height + g_line_thickness -
                g_left_arrowhead_rasterization.bitmap.width) / 2,
            skill_table_min_y + (g_table_row_height + g_line_thickness -
                g_left_arrowhead_rasterization.bitmap.rows) / 2, spend_die_border_color);
        int32_t tab_cell_min_x = client_rect.right - g_tab_width;
        uint32_t tab_cell_width = g_tab_width - g_line_thickness;
        int32_t tab_min_x = tab_cell_min_x - g_line_thickness;
        int32_t tab_min_y = -(int32_t)g_line_thickness;
        select_table_row(&g_window_rect, &g_selected_tab_index, ARRAY_COUNT(g_tab_names), tab_min_x,
            tab_min_y, g_tab_width, TABS_ID);
        for (size_t i = 0; i < ARRAY_COUNT(g_tab_names); ++i)
        {
            tab_min_y += g_table_row_height;
            draw_horizontally_centered_string(g_tab_names[i], &g_window_rect, tab_cell_min_x,
                tab_min_y - g_text_padding, tab_cell_width);
            draw_filled_rectangle(&g_window_rect, tab_cell_min_x, tab_min_y, tab_cell_width,
                g_line_thickness, g_black);
        }
        draw_filled_rectangle(&g_window_rect, tab_min_x, 0, g_line_thickness, client_rect.bottom,
            g_black);
        draw_filled_rectangle(&g_window_rect, tab_min_x, g_selected_tab_index * g_table_row_height,
            g_line_thickness, cell_height, g_dark_gray);
        draw_filled_rectangle(&g_window_rect, client_rect.right - g_line_thickness, 0,
            g_line_thickness, ARRAY_COUNT(g_tab_names) * g_table_row_height, g_black);
        StretchDIBits(device_context, 0, 0, g_window_rect.width, g_window_rect.height, 0, 0,
            client_rect.right - client_rect.left, client_rect.bottom - client_rect.top,
            g_pixels, &g_pixel_buffer_info, DIB_RGB_COLORS, SRCCOPY);
        g_left_mouse_button_changed_state = false;
        if (!g_left_mouse_button_is_down)
        {
            g_clicked_control_id = NULL_CONTROL_ID;
        }
    }
    return 0;
}