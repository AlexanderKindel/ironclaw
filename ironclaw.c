#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdbool.h>
#include <stdint.h>

#ifdef _DEBUG
#define ASSERT(condition) if (!(condition)) { *(int*)0 = 0; }
#else
#define ASSERT(condition)
#endif

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof(arr[0]))

#define UNPAREN(...) __VA_ARGS__
#define MAKE_ENUM(name, value) name,
#define MAKE_VALUE(name, value) UNPAREN value,

uint8_t g_unallocated_dice[] = { 1, 3, 2 };

#include "rulebook_data.c"

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
        COMMIT_PAGE(output_stack->cursor_max);
        output_stack->cursor_max = (uint8_t*)output_stack->cursor_max + g_page_size;
    }
}

void*allocate_stack_slot(Stack*output_stack, size_t slot_size, size_t alignment)
{
    void*slot = align_cursor(output_stack, alignment);
    allocate_unaligned_stack_slot(output_stack, slot_size);
    return slot;
}

typedef struct Interval
{
    int32_t min;
    int32_t length;
} Interval;

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

Color*g_pixels;
Interval g_window_x_extent;
Interval g_window_y_extent;

uint32_t g_line_thickness;
uint32_t g_table_row_height;

Color g_black = { .value = 0xff000000 };
Color g_dark_gray = { .value = 0xffb0b0b0 };
Color g_light_gray = { .value = 0xffd0d0d0 };
Color g_white = { .value = 0xffffffff };

void draw_filled_rectangle(Interval clip_x_extent, Interval clip_y_extent, int32_t min_x,
    int32_t min_y, uint32_t width, uint32_t height, Color color)
{
    int32_t max_x = min32(min_x + width, clip_x_extent.min + clip_x_extent.length);
    min_x = max32(min_x, clip_x_extent.min);
    int32_t max_y = min32(min_y + height, clip_y_extent.min + clip_y_extent.length);
    min_y = max32(min_y, clip_y_extent.min);
    for (int32_t y = min_y; y < max_y; ++y)
    {
        Color*row = g_pixels + y * g_window_x_extent.length;
        for (int32_t x = min_x; x < max_x; ++x)
        {
            row[x].value = color.value;
        }
    }
}

void draw_rectangle_outline(Interval clip_x_extent, Interval clip_y_extent, int32_t min_x,
    int32_t min_y, uint32_t width, uint32_t height, Color color)
{
    draw_filled_rectangle(clip_x_extent, clip_y_extent, min_x, min_y, width, g_line_thickness,
        color);
    draw_filled_rectangle(clip_x_extent, clip_y_extent, min_x + g_line_thickness, min_y + height,
        width, g_line_thickness, color);
    draw_filled_rectangle(clip_x_extent, clip_y_extent, min_x, min_y + g_line_thickness,
        g_line_thickness, height, color);
    draw_filled_rectangle(clip_x_extent, clip_y_extent, min_x + width, min_y, g_line_thickness,
        height, color);
}

typedef struct Grid
{
    size_t column_count;
    size_t row_count;
    int32_t*column_min_x_values;
    int32_t min_y;
} Grid;

int32_t get_rightmost_column_x(Grid*grid)
{
    return grid->column_min_x_values[grid->column_count];
}

uint32_t get_grid_width(Grid*grid)
{
    return get_rightmost_column_x(grid) - grid->column_min_x_values[0];
}

void draw_grid_dividers(Interval clip_x_extent, Interval clip_y_extent, Grid*grid)
{
    int32_t cell_min_y = grid->min_y + g_line_thickness;
    uint32_t height = g_table_row_height * grid->row_count - g_line_thickness;
    for (size_t column_index = 1; column_index < grid->column_count; ++column_index)
    {
        draw_filled_rectangle(clip_x_extent, clip_y_extent, grid->column_min_x_values[column_index],
            cell_min_y, g_line_thickness, height, g_black);
    }
    int32_t cell_min_x = grid->column_min_x_values[0] + g_line_thickness;
    int32_t row_min_y = grid->min_y;
    uint32_t cells_width = grid->column_min_x_values[grid->column_count] - cell_min_x;
    for (size_t row_index = 1; row_index < grid->row_count; ++row_index)
    {
        row_min_y += g_table_row_height;
        draw_filled_rectangle(clip_x_extent, clip_y_extent, cell_min_x, row_min_y, cells_width,
            g_line_thickness, g_black);
    }
}

void draw_grid(Interval clip_x_extent, Interval clip_y_extent, Grid*grid)
{
    draw_rectangle_outline(clip_x_extent, clip_y_extent, grid->column_min_x_values[0], grid->min_y,
        get_grid_width(grid), g_table_row_height * grid->row_count, g_black);
    draw_grid_dividers(clip_x_extent, clip_y_extent, grid);
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
Rasterization g_up_arrowhead_rasterization;
Rasterization g_right_arrowhead_rasterization;
Rasterization g_down_arrowhead_rasterization;
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

void draw_rasterization(Interval clip_x_extent, Interval clip_y_extent, Rasterization*rasterization,
    int32_t min_x, int32_t min_y, Color color)
{
    int32_t max_x_in_bitmap = min32(rasterization->bitmap.width,
        clip_x_extent.min + clip_x_extent.length - min_x);
    int32_t max_y_in_bitmap =
        min32(rasterization->bitmap.rows, clip_y_extent.min + clip_y_extent.length - min_y);
    Color*bitmap_top_left_in_window_buffer = g_pixels + min_y * g_window_x_extent.length + min_x;
    for (int32_t bitmap_y = max32(0, clip_y_extent.min - min_y); bitmap_y < max_y_in_bitmap;
        ++bitmap_y)
    {
        Color*bitmap_row_left_end_in_window_buffer =
            bitmap_top_left_in_window_buffer + bitmap_y * g_window_x_extent.length;
        for (int32_t bitmap_x = max32(0, clip_x_extent.min - min_x); bitmap_x < max_x_in_bitmap;
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

FT_Pos draw_string(char*string, Interval clip_x_extent, Interval clip_y_extent, int32_t origin_x,
    int32_t origin_y)
{
    FT_Pos advance = origin_x << 6;
    FT_UInt previous_glyph_index = FT_Get_Char_Index(g_face, string[0]);
    FT_Vector kerning_with_previous_glyph = { 0, 0 };
    for (char*character = string; *character; ++character)
    {
        advance += kerning_with_previous_glyph.x;
        Rasterization*rasterization = get_rasterization(*character);
        draw_rasterization(clip_x_extent, clip_y_extent, rasterization,
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

void draw_horizontally_centered_string(char*string, Interval clip_x_extent, Interval clip_y_extent,
    int32_t cell_min_x, int32_t text_origin_y, uint32_t cell_width)
{
    draw_string(string, clip_x_extent, clip_y_extent,
        cell_min_x + (cell_width - get_string_width(string)) / 2, text_origin_y);
}

void draw_uint8(Interval clip_x_extent, Interval clip_y_extent, uint8_t value, int32_t origin_x,
    int32_t origin_y)
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
    draw_string(string, clip_x_extent, clip_y_extent, origin_x, origin_y);
}

uint32_t g_text_lines_per_mouse_wheel_notch;
uint8_t g_clicked_control_id;
int32_t g_cursor_x;
int32_t g_cursor_y;
bool g_left_mouse_button_is_down;
bool g_left_mouse_button_changed_state;
bool g_shift_is_down;
int16_t g_120ths_of_mouse_wheel_notches_turned;

bool do_button_action(Interval clip_x_extent, Interval clip_y_extent, int32_t min_x, int32_t min_y,
    uint32_t width, uint32_t height, uint8_t control_id)
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
                draw_filled_rectangle(clip_x_extent, clip_y_extent, cell_min_x, cell_min_y,
                    width - g_line_thickness, height - g_line_thickness, g_white);
            }
        }
        else
        {
            draw_filled_rectangle(clip_x_extent, clip_y_extent, cell_min_x, cell_min_y,
                width - g_line_thickness, height - g_line_thickness, g_light_gray);
            if (g_left_mouse_button_changed_state && g_clicked_control_id == control_id)
            {
                g_clicked_control_id = 0;
                return true;
            }
        }
    }
    else if (g_clicked_control_id == control_id)
    {
        if (g_left_mouse_button_is_down)
        {
            uint32_t interior_dimension = g_table_row_height - g_line_thickness;
            draw_filled_rectangle(clip_x_extent, clip_y_extent, cell_min_x, cell_min_y,
                width - g_line_thickness, height - g_line_thickness, g_light_gray);
        }
        else if (g_left_mouse_button_changed_state)
        {
            g_clicked_control_id = 0;
        }
    }
    return false;
}

bool select_table_row(Interval clip_x_extent, Interval clip_y_extent, size_t*selected_row_index,
    size_t table_row_count, int32_t table_min_x, int32_t table_min_y, uint32_t table_width,
    uint8_t table_row_0_id)
{
    bool selection_changed = false;
    if (g_cursor_y >= clip_y_extent.min && g_cursor_y < clip_y_extent.min + clip_y_extent.length &&
        g_cursor_y >= table_min_y)
    {
        size_t row_index = (g_cursor_y - table_min_y) / g_table_row_height;
        if (row_index < table_row_count &&
            do_button_action(clip_x_extent, clip_y_extent, table_min_x,
                table_min_y + row_index * g_table_row_height, table_width, g_table_row_height,
                table_row_0_id + row_index))
        {
            *selected_row_index = row_index;
            selection_changed = true;
        }
        if (g_clicked_control_id >= table_row_0_id &&
            g_clicked_control_id < table_row_0_id + table_row_count)
        {
            do_button_action(clip_x_extent, clip_y_extent, table_min_x,
                table_min_y + (g_clicked_control_id - table_row_0_id) * g_table_row_height,
                table_width, g_table_row_height, g_clicked_control_id);
        }
    }
    return selection_changed;
}

typedef struct ScrollBar
{
    int32_t min_along_thickness;
    int32_t min_along_length;
    uint32_t length;
    uint32_t content_length;
    uint32_t content_offset;
    Interval viewport_extent_along_thickness;
    Interval viewport_extent_along_length;
    int32_t previous_cursor_lengthwise_coord;
    bool is_active;
} ScrollBar;

typedef struct ScrollGeometryData
{
    uint32_t trough_thickness;
    uint32_t thumb_thickness;
    uint32_t thumb_length;
    uint32_t max_thumb_offset;
    uint32_t max_viewport_offset;
} ScrollGeometryData;

ScrollGeometryData get_scroll_geometry_data(ScrollBar*scroll)
{
    ScrollGeometryData out;
    out.trough_thickness = g_table_row_height - g_line_thickness;
    uint32_t trough_length_without_border = scroll->length - 2 * g_line_thickness;
    out.thumb_thickness = out.trough_thickness - 2 * g_line_thickness;
    out.thumb_length = (trough_length_without_border *
        scroll->viewport_extent_along_length.length) / scroll->content_length;
    out.max_thumb_offset = trough_length_without_border - out.thumb_length;
    out.max_viewport_offset = scroll->content_length - scroll->viewport_extent_along_length.length;
    return out;
}

uint32_t get_thumb_offset(ScrollBar*scroll, ScrollGeometryData*data)
{
    return (data->max_thumb_offset * scroll->content_offset + data->max_viewport_offset / 2) /
        data->max_viewport_offset;
}

bool update_scroll_bar_offset(ScrollBar*scroll, int32_t cursor_coord_along_thickness,
    int32_t cursor_coord_along_length, uint32_t scroll_id, bool scroll_wheel_applies)
{
    if (scroll->is_active)
    {
        ASSERT(scroll->content_length >= scroll->viewport_extent_along_length.length);
        uint32_t old_content_offset = scroll->content_offset;
        ScrollGeometryData data = get_scroll_geometry_data(scroll);
        int32_t thumb_min_along_thickness = scroll->min_along_thickness + g_line_thickness;
        int32_t thumb_min_along_length = scroll->min_along_length + g_line_thickness;
        if (scroll_wheel_applies &&
            cursor_coord_along_thickness >= scroll->viewport_extent_along_thickness.min &&
            cursor_coord_along_thickness < scroll->viewport_extent_along_thickness.min + 
                scroll->viewport_extent_along_thickness.length &&
            cursor_coord_along_length >= scroll->viewport_extent_along_length.min &&
            cursor_coord_along_length < scroll->viewport_extent_along_length.min +
                scroll->viewport_extent_along_length.length)
        {
            scroll->content_offset = max32(0,
                scroll->content_offset - (g_120ths_of_mouse_wheel_notches_turned *
                (int32_t)(g_text_lines_per_mouse_wheel_notch * g_table_row_height)) / 120);
            g_120ths_of_mouse_wheel_notches_turned = 0;
        }
        if (g_left_mouse_button_is_down && g_clicked_control_id == scroll_id)
        {
            int32_t thumb_offset = get_thumb_offset(scroll, &data) + cursor_coord_along_length -
                scroll->previous_cursor_lengthwise_coord;
            if (thumb_offset > 0)
            {
                thumb_offset = min32(thumb_offset, data.max_thumb_offset);
            }
            else
            {
                thumb_offset = 0;
            }
            thumb_min_along_length += thumb_offset;
            scroll->content_offset = (data.max_viewport_offset * thumb_offset +
                data.max_thumb_offset / 2) / data.max_thumb_offset;
            scroll->previous_cursor_lengthwise_coord = cursor_coord_along_length;
        }
        else
        {
            scroll->content_offset = min32(scroll->content_offset, data.max_viewport_offset);
            thumb_min_along_length += get_thumb_offset(scroll, &data);
            if (g_left_mouse_button_is_down && g_left_mouse_button_changed_state &&
                cursor_coord_along_thickness >= thumb_min_along_thickness &&
                cursor_coord_along_thickness < thumb_min_along_thickness + data.thumb_thickness &&
                cursor_coord_along_length >= thumb_min_along_length &&
                cursor_coord_along_length < thumb_min_along_length + data.thumb_length)
            {
                g_clicked_control_id = scroll_id;
                scroll->previous_cursor_lengthwise_coord = cursor_coord_along_length;
            }
        }
        return old_content_offset != scroll->content_offset;
    }
    return false;
}

bool update_vertical_scroll_bar_offset(ScrollBar*scroll, uint32_t scroll_id)
{
    return update_scroll_bar_offset(scroll, g_cursor_x, g_cursor_y, scroll_id, !g_shift_is_down);
}

void draw_vertical_scroll_bar(Interval clip_x_extent, Interval clip_y_extent, ScrollBar*scroll)
{
    if (scroll->is_active)
    {
        ScrollGeometryData data = get_scroll_geometry_data(scroll);
        draw_filled_rectangle(clip_x_extent, clip_y_extent, scroll->min_along_thickness,
            scroll->min_along_length, data.trough_thickness, scroll->length, g_light_gray);
        draw_filled_rectangle(clip_x_extent, clip_y_extent,
            scroll->min_along_thickness + g_line_thickness,
            scroll->min_along_length + g_line_thickness + get_thumb_offset(scroll, &data),
            data.thumb_thickness, data.thumb_length, g_dark_gray);
    }
}