#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdbool.h>
#include <stdint.h>

uint8_t g_unallocated_dice[] = { 1, 3, 2 };

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof(arr[0]))

#define UNPAREN(...) __VA_ARGS__
#define MAKE_ENUM(name, value) name,
#define MAKE_VALUE(name, value) UNPAREN value,

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
    int32_t*column_min_x_values;
    int32_t min_y;
} Grid;

int32_t get_leftmost_column_x(Grid*grid)
{
    return grid->column_min_x_values[grid->column_count];
}

uint32_t get_grid_width(Grid*grid)
{
    return get_leftmost_column_x(grid) - grid->column_min_x_values[0];
}

void draw_grid_dividers(Rect*clip_rect, Grid*grid)
{
    int32_t cell_min_y = grid->min_y + g_line_thickness;
    uint32_t height = g_table_row_height * grid->row_count - g_line_thickness;
    for (size_t column_index = 1; column_index < grid->column_count; ++column_index)
    {
        draw_filled_rectangle(clip_rect, grid->column_min_x_values[column_index], cell_min_y,
            g_line_thickness, height, g_black);
    }
    int32_t cell_min_x = grid->column_min_x_values[0] + g_line_thickness;
    int32_t row_min_y = grid->min_y;
    uint32_t cells_width = grid->column_min_x_values[grid->column_count] - cell_min_x;
    for (size_t row_index = 1; row_index < grid->row_count; ++row_index)
    {
        row_min_y += g_table_row_height;
        draw_filled_rectangle(clip_rect, cell_min_x, row_min_y, cells_width, g_line_thickness,
            g_black);
    }
}

void draw_grid(Grid*grid)
{
    draw_rectangle_outline(grid->column_min_x_values[0], grid->min_y, get_grid_width(grid),
        g_table_row_height * grid->row_count, g_black);
    draw_grid_dividers(&g_window_rect, grid);
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
        for (int32_t bitmap_x = max32(0, clip_rect->min_x - min_x); bitmap_x < max_x_in_bitmap;
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
#define ALLOCATE_BUTTON_ID 0b1100000
#define DEALLOCATE_BUTTON_ID 0b10000000
#define TABS_ID 0b10100000

#define UNALLOCATED_DICE_TABLE_ID 0b11000000
#define TRAIT_TABLE_ID 0b100000000

#define UNALLOCATED_MARKS_TABLE_ID 0b11000000

uint32_t g_text_lines_per_mouse_wheel_notch;
uint32_t g_clicked_control_id = 0;
int32_t g_cursor_x;
int32_t g_cursor_y;
bool g_left_mouse_button_is_down = false;
bool g_left_mouse_button_changed_state = false;
int16_t g_120ths_of_mouse_wheel_notches_turned = 0;

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

bool select_table_row(Rect*clip_rect, size_t*selected_row_index, size_t table_row_count,
    int32_t table_min_x, int32_t table_min_y, uint32_t table_width, uint32_t table_id)
{
    bool selection_changed = false;
    if (g_cursor_y >= table_min_y)
    {
        size_t row_index = (g_cursor_y - table_min_y) / g_table_row_height;
        if (row_index < table_row_count &&
            do_button_action(clip_rect, table_id | row_index, table_min_x,
                table_min_y + row_index * g_table_row_height, table_width, g_table_row_height))
        {
            *selected_row_index = row_index;
            selection_changed = true;
        }
        if ((g_clicked_control_id & PARENT_CONTROL_ID_MASK) == table_id)
        {
            do_button_action(clip_rect, g_clicked_control_id, table_min_x,
                table_min_y + (g_clicked_control_id & CHILD_CONTROL_ID_MASK) * g_table_row_height,
                table_width, g_table_row_height);
        }
    }
    return selection_changed;
}

typedef struct ScrollOffset
{
    uint32_t previous_content_offset;
    uint32_t current_content_offset;
    int32_t previous_cursor_y;
} ScrollOffset;

void update_scroll_bar_offset(Rect*viewport, ScrollOffset*offset, uint32_t content_height,
    uint32_t scroll_id)
{
    uint32_t trough_width = g_table_row_height - g_line_thickness;
    int32_t thumb_min_x = viewport->min_x - trough_width;
    draw_filled_rectangle(&g_window_rect, thumb_min_x - g_line_thickness,
        viewport->min_y - g_line_thickness, g_table_row_height,
        viewport->height + 2 * g_line_thickness, g_light_gray);
    uint32_t thumb_width = trough_width - 2 * g_line_thickness;
    uint32_t thumb_height = (viewport->height * viewport->height) / content_height;
    uint32_t max_thumb_offset = viewport->height - thumb_height;
    uint32_t max_viewport_offset = content_height - viewport->height;
    int32_t thumb_min_y = viewport->min_y;
    if (g_cursor_x >= viewport->min_x && g_cursor_x < viewport->min_x + viewport->height &&
        g_cursor_y >= viewport->min_y && g_cursor_y < viewport->min_y + viewport->height)
    {
        offset->current_content_offset =
            max32(0, offset->current_content_offset - (g_120ths_of_mouse_wheel_notches_turned *
            (int32_t)(g_text_lines_per_mouse_wheel_notch * g_table_row_height)) / 120);
        offset->previous_content_offset = offset->current_content_offset;
    }
    if (g_left_mouse_button_is_down && g_clicked_control_id == scroll_id)
    {
        int32_t thumb_offset =
            (max_thumb_offset * offset->previous_content_offset + max_viewport_offset / 2) /
            max_viewport_offset + g_cursor_y - offset->previous_cursor_y;
        if (thumb_offset > 0)
        {
            thumb_offset = min32(thumb_offset, max_thumb_offset);
        }
        else
        {
            thumb_offset = 0;
        }
        thumb_min_y += thumb_offset;
        offset->current_content_offset =
            (max_viewport_offset * thumb_offset + max_thumb_offset / 2) / max_thumb_offset;
        offset->previous_content_offset = offset->current_content_offset;
        offset->previous_cursor_y = g_cursor_y;
    }
    else
    {
        offset->current_content_offset =
            min32(offset->current_content_offset, max_viewport_offset);
        thumb_min_y += (max_thumb_offset * offset->current_content_offset +
            max_viewport_offset / 2) / max_viewport_offset;
        if (g_left_mouse_button_is_down && g_left_mouse_button_changed_state &&
            g_cursor_x >= thumb_min_x && g_cursor_x < thumb_min_x + thumb_width &&
            g_cursor_y >= thumb_min_y && g_cursor_y < thumb_min_y + thumb_height)
        {
            g_clicked_control_id = scroll_id;
            offset->previous_content_offset = offset->current_content_offset;
            offset->previous_cursor_y = g_cursor_y;
        }
    }
    draw_filled_rectangle(&g_window_rect, thumb_min_x, thumb_min_y, thumb_width,
        thumb_height, g_dark_gray);
}

uint32_t g_skill_column_width;
uint32_t g_die_column_width;
size_t g_selected_skill_index = ARRAY_COUNT(g_skills);

ScrollOffset g_skill_table_scroll_offset;

size_t g_selected_die_denomination_index = ARRAY_COUNT(g_unallocated_dice);

uint32_t g_trait_column_width;
size_t g_selected_trait_index = ARRAY_COUNT(g_traits);

uint8_t g_unallocated_marks = 13;

#define TABS(macro)\
macro(TAB_ALLOCATE_DICE, ("Allocate Trait Dice"))\
macro(TAB_ALLOCATE_MARKS, ("Allocate Marks"))

enum TabIndex
{
    TABS(MAKE_ENUM)
};

char*g_tab_names[] = { TABS(MAKE_VALUE) };
uint32_t g_tab_width;
size_t g_selected_tab_index = 0;

void scale_window_contents(void*font_data, size_t font_data_size, uint16_t dpi)
{
    FT_New_Memory_Face(g_freetype_library, font_data, font_data_size, 0, &g_face);
    FT_Set_Char_Size(g_face, 0, g_message_font_height, 0, dpi);
    for (FT_ULong codepoint = FIRST_RASTERIZED_GLYPH; codepoint <= LAST_RASTERIZED_GLYPH;
        ++codepoint)
    {
        rasterize_glyph(codepoint, get_rasterization(codepoint));
    }
    rasterize_glyph(0x25b2, &g_up_arrowhead_rasterization);
    rasterize_glyph(0x25ba, &g_right_arrowhead_rasterization);
    rasterize_glyph(0x25bc, &g_down_arrowhead_rasterization);
    rasterize_glyph(0x25c4, &g_left_arrowhead_rasterization);
    g_line_thickness = get_rasterization('|')->bitmap.width;
    g_table_row_height =
        round26_6to_pixel(g_face->size->metrics.ascender - 2 * g_face->size->metrics.descender);
    g_text_padding = (g_table_row_height - get_rasterization('M')->bitmap.rows) / 2;
    g_table_row_height += g_line_thickness;
    g_skill_column_width = get_string_width("Skills");
    for (size_t i = 0; i < ARRAY_COUNT(g_skills); ++i)
    {
        g_skill_column_width = max32(g_skill_column_width, get_string_width(g_skills[i].name));
    }
    g_skill_column_width += g_line_thickness + 2 * g_text_padding;
    g_die_column_width = g_line_thickness + get_string_width("D12") + 2 * g_text_padding;
    g_trait_column_width = get_string_width("Traits");
    for (size_t i = 0; i < ARRAY_COUNT(g_traits); ++i)
    {
        g_trait_column_width = max32(g_trait_column_width, get_string_width(g_traits[i].name));
    }
    g_trait_column_width += g_line_thickness + 2 * g_text_padding;
    g_tab_width = g_line_thickness + 2 * g_text_padding +
        max32(get_string_width(g_tab_names[0]), get_string_width(g_tab_names[1]));
}

Rect g_skill_table_viewport;
bool g_skill_table_scroll_is_active;
int32_t g_skill_table_column_min_x_values[3 + ARRAY_COUNT(g_die_denominations)];
Grid g_skill_table;
Grid g_skill_table_dice_header;
int32_t g_allocate_button_min_x;
int32_t g_allocate_button_min_y;
Rasterization*g_allocate_button_arrow;
int32_t g_deallocate_button_min_x;
int32_t g_deallocate_button_min_y;
Rasterization*g_deallocate_button_arrow;

typedef union TabLayout
{
    struct
    {
        Grid unallocated_dice_table;
        Grid trait_table;
        int32_t trait_table_column_min_x_values[2 + ARRAY_COUNT(g_unallocated_dice)];
    };
    struct
    {
        int32_t unallocated_marks_display_min_x;
        int32_t unallocated_marks_display_min_y;
        uint32_t unallocated_marks_display_width;
    };
} TabLayout;

TabLayout g_layout;

void format_skill_table(int32_t min_x)
{
    uint32_t cell_height = g_table_row_height - g_line_thickness;
    g_skill_table.column_min_x_values[0] = min_x;
    g_skill_table_viewport.height =
        g_window_rect.height - (g_skill_table_viewport.min_y + g_table_row_height);
    uint32_t skill_table_cells_height =
        g_table_row_height * ARRAY_COUNT(g_skills) - g_line_thickness;
    g_skill_table_scroll_is_active = g_skill_table_viewport.height < skill_table_cells_height;
    if (g_skill_table_scroll_is_active)
    {
        g_skill_table.column_min_x_values[0] += cell_height;
    }
    else
    {
        g_skill_table_scroll_offset.current_content_offset = 0;
        g_skill_table_viewport.height = skill_table_cells_height;
    }
    g_skill_table_dice_header.min_y = cell_height;
    g_skill_table.column_min_x_values[1] =
        g_skill_table.column_min_x_values[0] + g_skill_column_width;
    g_skill_table.column_min_x_values[2] = g_skill_table.column_min_x_values[1] + g_line_thickness +
        get_string_width("Marks") + 2 * g_text_padding;
    for (size_t i = 2; i < g_skill_table.column_count; ++i)
    {
        g_skill_table.column_min_x_values[i + 1] =
            g_skill_table.column_min_x_values[i] + g_die_column_width;
    }
    g_skill_table_viewport.min_x = g_skill_table.column_min_x_values[0] + g_line_thickness;
    g_skill_table_viewport.min_y =
        g_skill_table_dice_header.min_y + g_table_row_height + g_line_thickness;
    g_skill_table_viewport.width = get_grid_width(&g_skill_table) - g_line_thickness;
}

void format_allocate_trait_dice_tab()
{
    g_allocate_button_arrow = &g_down_arrowhead_rasterization;
    g_deallocate_button_arrow = &g_up_arrowhead_rasterization;
    uint32_t cell_height = g_table_row_height - g_line_thickness;
    g_layout.trait_table.column_count = ARRAY_COUNT(g_layout.trait_table_column_min_x_values) - 1;
    g_layout.trait_table.row_count = ARRAY_COUNT(g_traits);
    g_layout.trait_table.column_min_x_values = g_layout.trait_table_column_min_x_values;
    g_layout.trait_table.column_min_x_values[0] = cell_height;
    g_layout.trait_table.column_min_x_values[1] =
        g_layout.trait_table.column_min_x_values[0] + g_trait_column_width;
    for (size_t i = 2; i <= g_layout.trait_table.column_count; ++i)
    {
        g_layout.trait_table.column_min_x_values[i] =
            g_layout.trait_table.column_min_x_values[i - 1] + g_die_column_width;
    }
    g_layout.unallocated_dice_table.column_count = ARRAY_COUNT(g_unallocated_dice);
    g_layout.unallocated_dice_table.row_count = 2;
    g_layout.unallocated_dice_table.column_min_x_values =
        g_layout.trait_table_column_min_x_values + 1;
    g_layout.unallocated_dice_table.min_y = cell_height + g_table_row_height;
    g_allocate_button_min_x = g_layout.unallocated_dice_table.column_min_x_values[0] +
        (get_grid_width(&g_layout.unallocated_dice_table) + g_line_thickness) / 2 -
        g_table_row_height;
    g_deallocate_button_min_x = g_allocate_button_min_x + g_table_row_height;
    format_skill_table(get_leftmost_column_x(&g_layout.trait_table) + g_table_row_height);
    g_allocate_button_min_y = g_layout.unallocated_dice_table.min_y +
        (g_layout.unallocated_dice_table.row_count + 1) * g_table_row_height;
    g_deallocate_button_min_y = g_allocate_button_min_y;
    g_layout.trait_table.min_y = g_allocate_button_min_y + 3 * g_table_row_height;
}

void format_allocate_marks_tab()
{
    g_allocate_button_arrow = &g_right_arrowhead_rasterization;
    g_deallocate_button_arrow = &g_left_arrowhead_rasterization;
    g_layout.unallocated_marks_display_min_x = g_table_row_height - g_line_thickness;
    g_layout.unallocated_marks_display_width = g_line_thickness +
        get_string_width("Unallocated Marks") + 2 * g_text_padding;
    g_allocate_button_min_x = g_layout.unallocated_marks_display_min_x +
        g_layout.unallocated_marks_display_width + g_table_row_height;
    g_deallocate_button_min_x = g_allocate_button_min_x;
    format_skill_table(g_allocate_button_min_x + 2 * g_table_row_height);
    g_layout.unallocated_marks_display_min_y = g_skill_table.min_y + g_table_row_height;
    g_allocate_button_min_y = g_skill_table.min_y;
    g_deallocate_button_min_y = g_layout.unallocated_marks_display_min_y;    
}

void format_window()
{
    switch (g_selected_tab_index)
    {
    case TAB_ALLOCATE_DICE:
        format_allocate_trait_dice_tab();
        break;
    case TAB_ALLOCATE_MARKS:
        format_allocate_marks_tab();
    }
}

#define INIT(copy_font_data_param, dpi)\
{\
    SET_MESSAGE_FONT_SIZE(dpi);\
    SET_PAGE_SIZE();\
    g_stack.start = RESERVE_MEMORY(UINT32_MAX);\
    g_stack.end = (uint8_t*)g_stack.start + UINT32_MAX;\
    g_stack.cursor = g_stack.start;\
    g_stack.cursor_max = g_stack.start;\
    g_skill_table_dice_header.column_count = ARRAY_COUNT(g_die_denominations);\
    g_skill_table_dice_header.row_count = 1;\
    g_skill_table_dice_header.column_min_x_values = g_skill_table_column_min_x_values + 2;\
    g_skill_table.column_count = ARRAY_COUNT(g_skill_table_column_min_x_values) - 1;\
    g_skill_table.row_count = ARRAY_COUNT(g_skills);\
    g_skill_table.column_min_x_values = g_skill_table_column_min_x_values;\
    FT_Init_FreeType(&g_freetype_library);\
    scale_window_contents(g_stack.start, COPY_FONT_DATA_TO_STACK_CURSOR(copy_font_data_param, dpi),\
        dpi);\
    format_allocate_trait_dice_tab();\
    SET_INITIAL_CURSOR_POSITION();\
}

void handle_message(void)
{
    for (size_t i = 0; i < g_window_rect.width * g_window_rect.height; ++i)
    {
        g_pixels[i].value = g_dark_gray.value;
    }
    int32_t tab_cell_min_x = g_window_rect.width - g_tab_width;
    uint32_t tab_cell_width = g_tab_width - g_line_thickness;
    int32_t tab_min_x = tab_cell_min_x - g_line_thickness;
    int32_t tab_min_y = -(int32_t)g_line_thickness;
    if (select_table_row(&g_window_rect, &g_selected_tab_index, ARRAY_COUNT(g_tab_names), tab_min_x,
        tab_min_y, g_tab_width, TABS_ID))
    {
        format_window();
    }
    for (size_t i = 0; i < ARRAY_COUNT(g_tab_names); ++i)
    {
        tab_min_y += g_table_row_height;
        draw_horizontally_centered_string(g_tab_names[i], &g_window_rect, tab_cell_min_x,
            tab_min_y - g_text_padding, tab_cell_width);
        draw_filled_rectangle(&g_window_rect, tab_cell_min_x, tab_min_y, tab_cell_width,
            g_line_thickness, g_black);
    }
    draw_filled_rectangle(&g_window_rect, tab_min_x, 0, g_line_thickness, g_window_rect.height,
        g_black);
    uint32_t cell_height = g_table_row_height - g_line_thickness;
    draw_filled_rectangle(&g_window_rect, tab_min_x, g_selected_tab_index * g_table_row_height,
        g_line_thickness, cell_height, g_dark_gray);
    draw_filled_rectangle(&g_window_rect, g_window_rect.width - g_line_thickness, 0,
        g_line_thickness, ARRAY_COUNT(g_tab_names) * g_table_row_height, g_black);
    if (g_skill_table_scroll_is_active)
    {
        update_scroll_bar_offset(&g_skill_table_viewport, &g_skill_table_scroll_offset,
            g_table_row_height * ARRAY_COUNT(g_skills) - g_line_thickness,
            SKILL_TABLE_SCROLL_BAR_THUMB_ID);
    }
    int32_t skill_table_frame_min_y = g_skill_table_dice_header.min_y + g_table_row_height;
    g_skill_table.min_y =
        skill_table_frame_min_y - g_skill_table_scroll_offset.current_content_offset;
    uint32_t skill_table_width = get_grid_width(&g_skill_table);
    Color allocate_button_border_color = g_light_gray;
    Color deallocate_button_border_color = g_light_gray;
    uint32_t unallocated_dice_table_height = 2 * g_table_row_height;
    if (g_selected_tab_index == 0)
    {
        int32_t unallocated_dice_table_cell_min_x =
            g_layout.unallocated_dice_table.column_min_x_values[0] + g_line_thickness;
        if (g_cursor_x >= unallocated_dice_table_cell_min_x)
        {
            size_t die_index =
                (g_cursor_x - unallocated_dice_table_cell_min_x) / g_die_column_width;
            if (die_index < g_layout.unallocated_dice_table.column_count)
            {
                if (do_button_action(&g_window_rect, UNALLOCATED_DICE_TABLE_ID | die_index,
                    g_layout.unallocated_dice_table.column_min_x_values[die_index],
                    skill_table_frame_min_y, g_die_column_width, unallocated_dice_table_height))
                {
                    g_selected_die_denomination_index = die_index;
                }
            }
            if ((g_clicked_control_id & PARENT_CONTROL_ID_MASK) == UNALLOCATED_DICE_TABLE_ID)
            {
                do_button_action(&g_window_rect, g_clicked_control_id,
                    g_layout.unallocated_dice_table.column_min_x_values[
                        g_clicked_control_id & CHILD_CONTROL_ID_MASK],
                    skill_table_frame_min_y, g_die_column_width, unallocated_dice_table_height);
            }
        }
        uint32_t die_cell_width = g_die_column_width - g_line_thickness;
        if (g_selected_die_denomination_index < g_layout.unallocated_dice_table.column_count)
        {
            draw_filled_rectangle(&g_window_rect, g_layout.unallocated_dice_table.
                column_min_x_values[g_selected_die_denomination_index] + g_line_thickness,
                g_skill_table_viewport.min_y + g_table_row_height, die_cell_width, cell_height,
                g_white);
        }
        select_table_row(&g_window_rect, &g_selected_trait_index, ARRAY_COUNT(g_traits),
            g_layout.trait_table.column_min_x_values[0], g_layout.trait_table.min_y,
            get_grid_width(&g_layout.trait_table), TRAIT_TABLE_ID);
        if (g_selected_trait_index < ARRAY_COUNT(g_traits))
        {
            draw_filled_rectangle(&g_window_rect, g_layout.trait_table.column_min_x_values[1],
                g_line_thickness + g_layout.trait_table.min_y +
                    g_selected_trait_index * g_table_row_height,
                g_layout.trait_table.column_min_x_values[g_layout.trait_table.column_count] -
                    g_layout.trait_table.column_min_x_values[1],
                cell_height, g_white);
        }
        if (g_selected_trait_index < ARRAY_COUNT(g_traits) && g_selected_die_denomination_index < 3)
        {
            uint8_t*selected_trait_dice =
                g_traits[g_selected_trait_index].dice + g_selected_die_denomination_index;
            if (g_unallocated_dice[g_selected_die_denomination_index])
            {
                if (do_button_action(&g_window_rect, ALLOCATE_BUTTON_ID,
                    g_allocate_button_min_x, g_allocate_button_min_y, g_table_row_height,
                    g_table_row_height))
                {
                    ++*selected_trait_dice;
                    --g_unallocated_dice[g_selected_die_denomination_index];
                }
                allocate_button_border_color = g_black;
            }
            if (*selected_trait_dice)
            {
                if (do_button_action(&g_window_rect, DEALLOCATE_BUTTON_ID,
                    g_deallocate_button_min_x, g_deallocate_button_min_y, g_table_row_height,
                    g_table_row_height))
                {
                    --*selected_trait_dice;
                    ++g_unallocated_dice[g_selected_die_denomination_index];
                }
                deallocate_button_border_color = g_black;
            }
        }
        draw_grid(&g_layout.unallocated_dice_table);
        int32_t cell_x = g_layout.unallocated_dice_table.column_min_x_values[0] + g_line_thickness;
        int32_t text_y = skill_table_frame_min_y - g_text_padding;
        uint32_t unallocated_dice_cells_width =
            get_grid_width(&g_layout.unallocated_dice_table) - g_line_thickness;
        draw_horizontally_centered_string("Unallocated Dice", &g_window_rect, cell_x, text_y,
            unallocated_dice_cells_width);
        text_y += g_table_row_height;
        for (size_t i = 0; i < g_layout.unallocated_dice_table.column_count; ++i)
        {
            draw_horizontally_centered_string(g_die_denominations[i], &g_window_rect, cell_x,
                text_y, die_cell_width);
            draw_uint8(&g_window_rect, g_unallocated_dice[i], cell_x + g_text_padding,
                text_y + g_table_row_height);
            cell_x += g_die_column_width;
        }
        draw_grid(&g_layout.trait_table);
        draw_grid(&(Grid) { 3, 1, g_layout.unallocated_dice_table.column_min_x_values,
            g_layout.trait_table.min_y - g_table_row_height });
        text_y = g_layout.trait_table.min_y - g_text_padding;
        draw_horizontally_centered_string("Traits", &g_window_rect,
            g_layout.trait_table.column_min_x_values[0], text_y,
            g_trait_column_width + g_line_thickness);
        draw_horizontally_centered_string("Dice", &g_window_rect,
            g_layout.unallocated_dice_table.column_min_x_values[0] + g_line_thickness,
            text_y - g_table_row_height, unallocated_dice_cells_width);
        for (size_t i = 0; i < 3; ++i)
        {
            draw_horizontally_centered_string(g_die_denominations[i], &g_window_rect,
                g_layout.unallocated_dice_table.column_min_x_values[i] + g_line_thickness, text_y,
                die_cell_width);
        }
        for (size_t i = 0; i < g_layout.trait_table.row_count; ++i)
        {
            text_y += g_table_row_height;
            Trait*trait = g_traits + i;
            draw_string(trait->name, &g_window_rect,
                g_layout.trait_table.column_min_x_values[0] + g_line_thickness + g_text_padding,
                text_y);
            for (size_t die_index = 0; die_index < ARRAY_COUNT(trait->dice); ++die_index)
            {
                draw_uint8(&g_window_rect, trait->dice[die_index],
                    g_layout.unallocated_dice_table.column_min_x_values[die_index] +
                        g_line_thickness + g_text_padding, text_y);
            }
        }
    }
    else
    {
        select_table_row(&g_skill_table_viewport, &g_selected_skill_index, ARRAY_COUNT(g_skills),
            g_skill_table.column_min_x_values[0], g_skill_table.min_y, skill_table_width,
            SKILL_TABLE_ID);
        if (g_selected_skill_index < ARRAY_COUNT(g_skills))
        {
            draw_filled_rectangle(&g_skill_table_viewport,
                g_line_thickness + g_skill_table.column_min_x_values[1],
                g_line_thickness + g_skill_table.min_y +
                    g_selected_skill_index * g_table_row_height,
                g_skill_table.column_min_x_values[2] -
                    (g_skill_table.column_min_x_values[1] + g_line_thickness),
                cell_height, g_white);
        }
        if (g_selected_skill_index < ARRAY_COUNT(g_skills))
        {
            Skill*selected_skill = g_skills + g_selected_skill_index;
            if (g_unallocated_marks && selected_skill->mark_count < 3)
            {
                if (do_button_action(&g_window_rect, ALLOCATE_BUTTON_ID, g_allocate_button_min_x,
                    g_allocate_button_min_y, g_table_row_height, g_table_row_height))
                {
                    ++selected_skill->mark_count;
                    --g_unallocated_marks;
                }
                allocate_button_border_color = g_black;
            }
            if (selected_skill->mark_count)
            {
                if (do_button_action(&g_window_rect, DEALLOCATE_BUTTON_ID,
                    g_deallocate_button_min_x, g_deallocate_button_min_y, g_table_row_height,
                    g_table_row_height))
                {
                    --selected_skill->mark_count;
                    ++g_unallocated_marks;
                }
                deallocate_button_border_color = g_black;
            }
        }
        draw_rectangle_outline(g_layout.unallocated_marks_display_min_x,
            g_layout.unallocated_marks_display_min_y, g_layout.unallocated_marks_display_width,
            g_table_row_height, g_black);
        int32_t text_x =
            g_layout.unallocated_marks_display_min_x + g_line_thickness + g_text_padding;
        int32_t text_y = g_deallocate_button_min_y - g_text_padding;
        draw_string("Unallocated Marks", &g_window_rect, text_x, text_y);
        draw_uint8(&g_window_rect, g_unallocated_marks, text_x, text_y + g_table_row_height);
    }
    draw_grid(&g_skill_table_dice_header);
    draw_horizontally_centered_string("Dice", &g_window_rect,
        g_skill_table.column_min_x_values[2] + g_line_thickness, cell_height - g_text_padding,
        get_grid_width(&g_skill_table_dice_header) - g_line_thickness);
    uint32_t die_column_width_with_both_borders = g_die_column_width + g_line_thickness;
    int32_t text_y = skill_table_frame_min_y - g_text_padding;
    for (size_t i = 2; i < g_skill_table.column_count; ++i)
    {
        draw_horizontally_centered_string(g_die_denominations[i - 2], &g_window_rect,
            g_skill_table.column_min_x_values[i], text_y, die_column_width_with_both_borders);
    }
    draw_grid_dividers(&g_skill_table_viewport, &g_skill_table);
    draw_rectangle_outline(g_skill_table.column_min_x_values[0], skill_table_frame_min_y,
        skill_table_width, g_skill_table_viewport.height + g_line_thickness, g_black);
    draw_horizontally_centered_string("Skills", &g_window_rect,
        g_skill_table.column_min_x_values[0], text_y,
        g_skill_table.column_min_x_values[1] + g_line_thickness -
            g_skill_table.column_min_x_values[0]);
    draw_string("Marks", &g_window_rect,
        g_skill_table.column_min_x_values[1] + g_line_thickness + g_text_padding, text_y);
    int32_t skill_text_x = g_skill_table.column_min_x_values[0] + g_line_thickness + g_text_padding;
    text_y = g_skill_table.min_y - g_text_padding;
    for (size_t i = 0; i < ARRAY_COUNT(g_skills); ++i)
    {
        text_y += g_table_row_height;
        Skill*skill = g_skills + i;
        draw_string(skill->name, &g_skill_table_viewport, skill_text_x, text_y);
        draw_uint8(&g_skill_table_viewport, skill->mark_count,
            g_skill_table.column_min_x_values[1] + g_line_thickness + g_text_padding, text_y);
        uint8_t die_counts[ARRAY_COUNT(g_die_denominations)] = { 0 };
        if (skill->mark_count)
        {
            ++die_counts[skill->mark_count - 1];
        }
        for (size_t die_index = 0; die_index < ARRAY_COUNT(g_die_denominations); ++die_index)
        {
            draw_uint8(&g_skill_table_viewport, die_counts[die_index],
                g_skill_table.column_min_x_values[2 + die_index] + g_line_thickness +
                    g_text_padding,
                text_y);
        }
    }
    if (allocate_button_border_color.value == g_black.value)
    {
        draw_rectangle_outline(g_deallocate_button_min_x, g_deallocate_button_min_y,
            g_table_row_height, g_table_row_height, deallocate_button_border_color);
        draw_rectangle_outline(g_allocate_button_min_x, g_allocate_button_min_y, g_table_row_height,
            g_table_row_height, allocate_button_border_color);
    }
    else
    {
        draw_rectangle_outline(g_allocate_button_min_x, g_allocate_button_min_y, g_table_row_height,
            g_table_row_height, allocate_button_border_color);
        draw_rectangle_outline(g_deallocate_button_min_x, g_deallocate_button_min_y,
            g_table_row_height, g_table_row_height, deallocate_button_border_color);
    }
    draw_rasterization(&g_window_rect, g_deallocate_button_arrow,
        g_deallocate_button_min_x + (g_table_row_height + g_line_thickness -
            g_right_arrowhead_rasterization.bitmap.width) / 2,
        g_deallocate_button_min_y + (g_table_row_height + g_line_thickness -
            g_right_arrowhead_rasterization.bitmap.rows) / 2, deallocate_button_border_color);
    draw_rasterization(&g_window_rect, g_allocate_button_arrow,
        g_allocate_button_min_x + (g_table_row_height + g_line_thickness -
            g_left_arrowhead_rasterization.bitmap.width) / 2,
        g_allocate_button_min_y + (g_table_row_height + g_line_thickness -
            g_left_arrowhead_rasterization.bitmap.rows) / 2, allocate_button_border_color);
    g_left_mouse_button_changed_state = false;
    g_120ths_of_mouse_wheel_notches_turned = 0;
    if (!g_left_mouse_button_is_down)
    {
        g_clicked_control_id = NULL_CONTROL_ID;
    }
}