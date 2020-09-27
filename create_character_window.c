#include "ironclaw.c"

#define TABS(macro)\
macro(TAB_ALLOCATE_DICE, ("Allocate Trait Dice"))\
macro(TAB_ALLOCATE_MARKS, ("Allocate Marks"))

enum TabIndex
{
    TABS(MAKE_ENUM)
};

char*g_tab_names[] = { TABS(MAKE_VALUE) };

enum WindowControlID
{
    SKILL_TABLE_SCROLL_ID = 1,
    ALLOCATE_BUTTON_ID,
    DEALLOCATE_BUTTON_ID,
    TAB_0_ID,
    WINDOW_VERTCAL_SCROLL_ID = TAB_0_ID + ARRAY_COUNT(g_tab_names),
    WINDOW_HORIZONTAL_SCROLL_ID,
    MAX_WINDOW_CONTROL_ID
};

ScrollBar g_window_horizontal_scroll;
ScrollBar g_vertical_scrolls[2];

#define SKILL_TABLE_SCROLL g_vertical_scrolls[0]
#define WINDOW_VERTICAL_SCROLL g_vertical_scrolls[1]

Grid g_skill_table;
Grid g_skill_table_dice_header;
int32_t g_skill_table_column_min_x_values[3 + ARRAY_COUNT(g_die_denominations)];
int32_t g_skill_table_frame_min_y;
uint32_t g_skill_column_width;
uint32_t g_die_column_width;
int32_t g_allocate_button_min_x;
int32_t g_allocate_button_min_y;
Rasterization*g_allocate_button_arrow;
int32_t g_deallocate_button_min_x;
int32_t g_deallocate_button_min_y;
Rasterization*g_deallocate_button_arrow;

int32_t g_tab_min_x;
uint32_t g_tab_width;
size_t g_selected_tab_index = 0;

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

enum AllocateTraitDiceTabID
{
    UNALLOCATED_DICE_TABLE_COLUMN_0_ID = MAX_WINDOW_CONTROL_ID,
    TRAIT_TABLE_ROW_0_ID = UNALLOCATED_DICE_TABLE_COLUMN_0_ID + ARRAY_COUNT(g_unallocated_dice)
};

size_t g_selected_die_denomination_index = ARRAY_COUNT(g_unallocated_dice);
size_t g_selected_trait_index = ARRAY_COUNT(g_traits);
uint32_t g_trait_column_width;

enum AllocateMarksTabID
{
    UNALLOCATED_MARKS_ID = MAX_WINDOW_CONTROL_ID,
    SKILL_TABLE_ROW_0_ID
};

size_t g_selected_skill_index = ARRAY_COUNT(g_skills);
uint8_t g_unallocated_marks = 13;

void scale_graphics_to_dpi(void*font_data, size_t font_data_size, uint16_t dpi)
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
    WINDOW_VERTICAL_SCROLL.viewport_extent_along_thickness.min = g_table_row_height;
    SKILL_TABLE_SCROLL.content_length =
        g_table_row_height * ARRAY_COUNT(g_skills) - g_line_thickness;
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

void reset_widget_x_positions(void)
{
    uint32_t cell_height = g_table_row_height - g_line_thickness;
    if (g_selected_tab_index == TAB_ALLOCATE_DICE)
    {
        g_layout.trait_table.column_min_x_values[0] =
            cell_height - g_window_horizontal_scroll.content_offset;
        if (WINDOW_VERTICAL_SCROLL.is_active)
        {
            g_layout.trait_table.column_min_x_values[0] += cell_height;
        }
        g_layout.trait_table.column_min_x_values[1] =
            g_layout.trait_table.column_min_x_values[0] + g_trait_column_width;
        for (size_t i = 2; i <= g_layout.trait_table.column_count; ++i)
        {
            g_layout.trait_table.column_min_x_values[i] =
                g_layout.trait_table.column_min_x_values[i - 1] + g_die_column_width;
        }
        g_deallocate_button_min_x = g_layout.unallocated_dice_table.column_min_x_values[0] +
            (get_grid_width(&g_layout.unallocated_dice_table) + g_line_thickness) / 2;
        g_allocate_button_min_x = g_deallocate_button_min_x - g_table_row_height;
        g_skill_table.column_min_x_values[0] =
            get_rightmost_column_x(&g_layout.trait_table) + g_table_row_height;
    }
    else
    {
        g_layout.unallocated_marks_display_min_x =
            cell_height - g_window_horizontal_scroll.content_offset;
        if (WINDOW_VERTICAL_SCROLL.is_active)
        {
            g_layout.unallocated_marks_display_min_x += cell_height;
        }
        g_allocate_button_min_x = g_layout.unallocated_marks_display_min_x +
            g_layout.unallocated_marks_display_width + g_table_row_height;
        g_deallocate_button_min_x = g_allocate_button_min_x;
        g_skill_table.column_min_x_values[0] = g_allocate_button_min_x + 2 * g_table_row_height;
    }
    if (SKILL_TABLE_SCROLL.is_active)
    {
        SKILL_TABLE_SCROLL.min_along_thickness = g_skill_table.column_min_x_values[0];
        g_skill_table.column_min_x_values[0] += cell_height;
    }
    g_skill_table.column_min_x_values[1] =
        g_skill_table.column_min_x_values[0] + g_skill_column_width;
    g_skill_table.column_min_x_values[2] = g_skill_table.column_min_x_values[1] +
        g_line_thickness + get_string_width("Marks") + 2 * g_text_padding;
    for (size_t i = 2; i < g_skill_table.column_count; ++i)
    {
        g_skill_table.column_min_x_values[i + 1] =
            g_skill_table.column_min_x_values[i] + g_die_column_width;
    }
    SKILL_TABLE_SCROLL.viewport_extent_along_thickness.min =
        g_skill_table.column_min_x_values[0] + g_line_thickness;
    if (g_window_horizontal_scroll.is_active)
    {
        g_tab_min_x = get_rightmost_column_x(&g_skill_table) + g_table_row_height;
    }
    else
    {
        g_tab_min_x = g_window_x_extent.length - (g_tab_width + g_line_thickness);
    }
}

void reset_widget_y_positions(void)
{
    g_skill_table_dice_header.min_y =
        g_table_row_height - g_line_thickness - WINDOW_VERTICAL_SCROLL.content_offset;
    g_skill_table_frame_min_y = g_skill_table_dice_header.min_y + g_table_row_height;
    if (SKILL_TABLE_SCROLL.is_active)
    {
        SKILL_TABLE_SCROLL.min_along_length = g_skill_table_frame_min_y;
    }
    SKILL_TABLE_SCROLL.viewport_extent_along_length.min =
        g_skill_table_frame_min_y + g_line_thickness;
    g_skill_table.min_y = g_skill_table_frame_min_y - SKILL_TABLE_SCROLL.content_offset;
    if (g_selected_tab_index == TAB_ALLOCATE_DICE)
    {
        g_layout.unallocated_dice_table.min_y = g_skill_table_frame_min_y;
        g_allocate_button_min_y = g_skill_table_frame_min_y + 3 * g_table_row_height;
        g_deallocate_button_min_y = g_allocate_button_min_y;
        g_layout.trait_table.min_y = g_allocate_button_min_y + 3 * g_table_row_height;
    }
    else
    {
        g_allocate_button_min_y = g_skill_table_frame_min_y;
        g_deallocate_button_min_y = g_allocate_button_min_y + g_table_row_height;
        g_layout.unallocated_marks_display_min_y = g_deallocate_button_min_y;
    }
}

void reset_widget_positions(void)
{
    reset_widget_x_positions();
    reset_widget_y_positions();
}

void reformat_widgets_on_window_resize()
{
    uint32_t cell_height = g_table_row_height - g_line_thickness;
    uint32_t window_scroll_viewport_heights_at_which_vertical_scrolls_activate[2];
    int32_t unscrolled_skill_table_frame_min_y = cell_height + g_table_row_height;
    window_scroll_viewport_heights_at_which_vertical_scrolls_activate[0] =
        unscrolled_skill_table_frame_min_y + (1 + g_skill_table.row_count) * g_table_row_height;
    if (g_selected_tab_index == TAB_ALLOCATE_DICE)
    {
        g_window_horizontal_scroll.content_length = get_grid_width(&g_layout.trait_table) +
            get_grid_width(&g_skill_table) + 3 * g_table_row_height + g_tab_width;
    }
    else
    {
        g_window_horizontal_scroll.content_length = g_layout.unallocated_marks_display_width +
            5 * g_table_row_height + get_grid_width(&g_skill_table) + g_tab_width;
    }
    window_scroll_viewport_heights_at_which_vertical_scrolls_activate[1] =
        WINDOW_VERTICAL_SCROLL.content_length;
    for (size_t i = 0; i < ARRAY_COUNT(g_vertical_scrolls); ++i)
    {
        g_vertical_scrolls[i].is_active = false;
    }
    g_window_horizontal_scroll.is_active = false;
    uint32_t window_content_width = g_window_horizontal_scroll.content_length;
    uint32_t window_scroll_viewport_height = g_window_y_extent.length;
    size_t vertical_scroll_index = 0;
    while (true)
    {
        while (true)
        {
            if (vertical_scroll_index == ARRAY_COUNT(g_vertical_scrolls))
            {
                if (window_content_width > g_window_x_extent.length)
                {
                    g_window_horizontal_scroll.is_active = true;
                }
                goto scroll_activity_states_are_set;
            }
            if (window_scroll_viewport_heights_at_which_vertical_scrolls_activate
                [vertical_scroll_index] > window_scroll_viewport_height)
            {
                g_vertical_scrolls[vertical_scroll_index].is_active = true;
                window_content_width += cell_height;
                ++vertical_scroll_index;
            }
            else
            {
                break;
            }
        }
        if (g_window_horizontal_scroll.is_active)
        {
            goto scroll_activity_states_are_set;
        }
        else if (window_content_width > g_window_x_extent.length)
        {
            g_window_horizontal_scroll.is_active = true;
            window_scroll_viewport_height -= cell_height;
        }
        else
        {
            ++vertical_scroll_index;
        }
    }
scroll_activity_states_are_set:
    g_window_horizontal_scroll.length = g_window_x_extent.length;
    g_window_horizontal_scroll.viewport_extent_along_thickness.length = g_window_y_extent.length;
    if (g_window_horizontal_scroll.is_active)
    {
        g_window_horizontal_scroll.min_along_thickness = g_window_y_extent.length - cell_height;
        g_window_horizontal_scroll.viewport_extent_along_thickness.length -= cell_height;
    }
    else
    {
        g_window_horizontal_scroll.content_offset = 0;
    }
    if (WINDOW_VERTICAL_SCROLL.is_active)
    {
        g_window_horizontal_scroll.min_along_length = cell_height;
        g_window_horizontal_scroll.length -= cell_height;
        WINDOW_VERTICAL_SCROLL.length =
            g_window_horizontal_scroll.viewport_extent_along_thickness.length;
        WINDOW_VERTICAL_SCROLL.viewport_extent_along_length.length = WINDOW_VERTICAL_SCROLL.length;
    }
    else
    {
        g_window_horizontal_scroll.min_along_length = 0;
        WINDOW_VERTICAL_SCROLL.content_offset = 0;
    }
    if (SKILL_TABLE_SCROLL.is_active)
    {
        g_window_horizontal_scroll.content_length += cell_height;
        SKILL_TABLE_SCROLL.viewport_extent_along_length.length =
            max32(g_window_horizontal_scroll.viewport_extent_along_thickness.length -
                g_table_row_height, WINDOW_VERTICAL_SCROLL.content_length - g_table_row_height) -
                (unscrolled_skill_table_frame_min_y + g_line_thickness);
        SKILL_TABLE_SCROLL.length =
            SKILL_TABLE_SCROLL.viewport_extent_along_length.length + 2 * g_line_thickness;
    }
    else
    {
        SKILL_TABLE_SCROLL.content_offset = 0;
        SKILL_TABLE_SCROLL.viewport_extent_along_length.length = SKILL_TABLE_SCROLL.content_length;
    }
    g_window_horizontal_scroll.viewport_extent_along_length.min =
        g_window_horizontal_scroll.min_along_length;
    g_window_horizontal_scroll.viewport_extent_along_length.length =
        g_window_horizontal_scroll.length;
    WINDOW_VERTICAL_SCROLL.viewport_extent_along_thickness.length =
        g_window_horizontal_scroll.viewport_extent_along_length.length;
    reset_widget_positions();
}

void select_allocate_trait_dice_tab(void)
{
    g_allocate_button_arrow = &g_down_arrowhead_rasterization;
    g_deallocate_button_arrow = &g_up_arrowhead_rasterization;
    g_layout.unallocated_dice_table.column_count = ARRAY_COUNT(g_unallocated_dice);
    g_layout.unallocated_dice_table.row_count = 2;
    g_layout.unallocated_dice_table.column_min_x_values =
        g_layout.trait_table_column_min_x_values + 1;
    g_layout.trait_table.column_count = ARRAY_COUNT(g_layout.trait_table_column_min_x_values) - 1;
    g_layout.trait_table.row_count = ARRAY_COUNT(g_traits);
    g_layout.trait_table.column_min_x_values = g_layout.trait_table_column_min_x_values;
    WINDOW_VERTICAL_SCROLL.content_length =
        g_line_thickness + (g_layout.trait_table.row_count + 9) * g_table_row_height;
    reformat_widgets_on_window_resize();
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
    scale_graphics_to_dpi(g_stack.start, COPY_FONT_DATA_TO_STACK_CURSOR(copy_font_data_param, dpi),\
        dpi);\
    select_allocate_trait_dice_tab();\
    SKILL_TABLE_SCROLL.viewport_extent_along_thickness.length =\
        get_grid_width(&g_skill_table) - g_line_thickness;\
    SET_INITIAL_CURSOR_POSITION();\
}

void handle_message(void)
{
    for (size_t i = 0; i < g_window_x_extent.length * g_window_y_extent.length; ++i)
    {
        g_pixels[i].value = g_dark_gray.value;
    }
    uint32_t cell_height = g_table_row_height - g_line_thickness;
    if (update_vertical_scroll_bar_offset(&SKILL_TABLE_SCROLL, SKILL_TABLE_SCROLL_ID))
    {
        g_skill_table.min_y = g_skill_table_frame_min_y - SKILL_TABLE_SCROLL.content_offset;
    }
    if (update_scroll_bar_offset(&g_window_horizontal_scroll, g_cursor_y, g_cursor_x,
        WINDOW_HORIZONTAL_SCROLL_ID, g_shift_is_down))
    {
        reset_widget_x_positions();
    }
    if (update_vertical_scroll_bar_offset(&WINDOW_VERTICAL_SCROLL, WINDOW_VERTCAL_SCROLL_ID))
    {
        reset_widget_y_positions();
    }
    draw_vertical_scroll_bar(g_window_x_extent, g_window_y_extent, &WINDOW_VERTICAL_SCROLL);
    if (g_window_horizontal_scroll.is_active)
    {
        ScrollGeometryData data = get_scroll_geometry_data(&g_window_horizontal_scroll);
        draw_filled_rectangle(g_window_x_extent, g_window_y_extent,
            g_window_horizontal_scroll.min_along_length,
            g_window_horizontal_scroll.min_along_thickness, g_window_horizontal_scroll.length,
            data.trough_thickness, g_light_gray);
        draw_filled_rectangle(g_window_x_extent, g_window_y_extent,
            g_window_horizontal_scroll.min_along_length + g_line_thickness +
                get_thumb_offset(&g_window_horizontal_scroll, &data),
            g_window_horizontal_scroll.min_along_thickness + g_line_thickness, data.thumb_length,
            data.thumb_thickness, g_dark_gray);
    }
    draw_vertical_scroll_bar(g_window_horizontal_scroll.viewport_extent_along_length,
        g_window_horizontal_scroll.viewport_extent_along_thickness, &SKILL_TABLE_SCROLL);
    int32_t tab_cell_min_x = g_tab_min_x + g_line_thickness;
    uint32_t tab_cell_width = g_tab_width - g_line_thickness;
    int32_t tab_min_y = -(g_line_thickness + WINDOW_VERTICAL_SCROLL.content_offset);
    if (select_table_row(g_window_horizontal_scroll.viewport_extent_along_length,
        g_window_horizontal_scroll.viewport_extent_along_thickness, &g_selected_tab_index,
        ARRAY_COUNT(g_tab_names), g_tab_min_x, tab_min_y, g_tab_width, TAB_0_ID))
    {
        if (g_selected_tab_index == TAB_ALLOCATE_DICE)
        {
            select_allocate_trait_dice_tab();
        }
        else
        {
            g_allocate_button_arrow = &g_right_arrowhead_rasterization;
            g_deallocate_button_arrow = &g_left_arrowhead_rasterization;
            g_layout.unallocated_marks_display_width =
                g_line_thickness + get_string_width("Unallocated Marks") + 2 * g_text_padding;
            WINDOW_VERTICAL_SCROLL.content_length = cell_height + 4 * g_table_row_height;
            reformat_widgets_on_window_resize();
        }
    }
    for (size_t i = 0; i < ARRAY_COUNT(g_tab_names); ++i)
    {
        tab_min_y += g_table_row_height;
        draw_horizontally_centered_string(g_tab_names[i],
            g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness, tab_cell_min_x,
            tab_min_y - g_text_padding, tab_cell_width);
        draw_filled_rectangle(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness, tab_cell_min_x, tab_min_y,
            tab_cell_width, g_line_thickness, g_black);
    }
    draw_filled_rectangle(g_window_horizontal_scroll.viewport_extent_along_length,
        g_window_horizontal_scroll.viewport_extent_along_thickness, g_tab_min_x, 0,
        g_line_thickness, g_window_y_extent.length, g_black);
    draw_filled_rectangle(g_window_horizontal_scroll.viewport_extent_along_length,
        g_window_horizontal_scroll.viewport_extent_along_thickness, g_tab_min_x,
        g_selected_tab_index * g_table_row_height -
            WINDOW_VERTICAL_SCROLL.content_offset,
        g_line_thickness, cell_height, g_dark_gray);
    draw_filled_rectangle(g_window_horizontal_scroll.viewport_extent_along_length,
        g_window_horizontal_scroll.viewport_extent_along_thickness, g_tab_min_x + g_tab_width,
        -WINDOW_VERTICAL_SCROLL.content_offset, g_line_thickness,
        ARRAY_COUNT(g_tab_names) * g_table_row_height, g_black);
    uint32_t skill_table_width = get_grid_width(&g_skill_table);
    Color allocate_button_border_color = g_light_gray;
    Color deallocate_button_border_color = g_light_gray;
    uint32_t unallocated_dice_table_height = 2 * g_table_row_height;
    Interval clipped_skill_table_viewport_width;
    clipped_skill_table_viewport_width.min =
        max32(SKILL_TABLE_SCROLL.viewport_extent_along_thickness.min, 0);
    clipped_skill_table_viewport_width.length =
        min32(g_window_x_extent.length, SKILL_TABLE_SCROLL.viewport_extent_along_thickness.min +
            SKILL_TABLE_SCROLL.viewport_extent_along_thickness.length) -
        clipped_skill_table_viewport_width.min;
    Interval clipped_skill_table_viewport_height;
    clipped_skill_table_viewport_height.min =
        max32(SKILL_TABLE_SCROLL.viewport_extent_along_length.min, 0);
    clipped_skill_table_viewport_height.length =
        min32(g_window_horizontal_scroll.viewport_extent_along_thickness.length,
            SKILL_TABLE_SCROLL.viewport_extent_along_length.min +
                SKILL_TABLE_SCROLL.viewport_extent_along_length.length) -
        clipped_skill_table_viewport_height.min;
    if (g_selected_tab_index == TAB_ALLOCATE_DICE)
    {
        int32_t unallocated_dice_table_cell_min_x =
            g_layout.unallocated_dice_table.column_min_x_values[0] + g_line_thickness;
        if (g_cursor_x >= unallocated_dice_table_cell_min_x)
        {
            size_t die_index =
                (g_cursor_x - unallocated_dice_table_cell_min_x) / g_die_column_width;
            if (die_index < g_layout.unallocated_dice_table.column_count)
            {
                if (do_button_action(g_window_horizontal_scroll.viewport_extent_along_length,
                    g_window_horizontal_scroll.viewport_extent_along_thickness,
                    g_layout.unallocated_dice_table.column_min_x_values[die_index],
                    g_skill_table_frame_min_y, g_die_column_width, unallocated_dice_table_height,
                    UNALLOCATED_DICE_TABLE_COLUMN_0_ID + die_index))
                {
                    g_selected_die_denomination_index = die_index;
                }
            }
            if (g_clicked_control_id >= UNALLOCATED_DICE_TABLE_COLUMN_0_ID &&
                g_clicked_control_id < UNALLOCATED_DICE_TABLE_COLUMN_0_ID +
                    ARRAY_COUNT(g_unallocated_dice))
            {
                do_button_action(g_window_horizontal_scroll.viewport_extent_along_length,
                    g_window_horizontal_scroll.viewport_extent_along_thickness,
                    g_layout.unallocated_dice_table.column_min_x_values[
                        g_clicked_control_id - UNALLOCATED_DICE_TABLE_COLUMN_0_ID],
                    g_skill_table_frame_min_y, g_die_column_width, unallocated_dice_table_height,
                    g_clicked_control_id);
            }
        }
        uint32_t die_cell_width = g_die_column_width - g_line_thickness;
        if (g_selected_die_denomination_index < g_layout.unallocated_dice_table.column_count)
        {
            draw_filled_rectangle(g_window_horizontal_scroll.viewport_extent_along_length,
                g_window_horizontal_scroll.viewport_extent_along_thickness,
                g_layout.unallocated_dice_table.
                    column_min_x_values[g_selected_die_denomination_index] + g_line_thickness,
                g_skill_table_frame_min_y + g_line_thickness + g_table_row_height, die_cell_width,
                cell_height, g_white);
        }
        select_table_row(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness, &g_selected_trait_index,
            ARRAY_COUNT(g_traits), g_layout.trait_table.column_min_x_values[0],
            g_layout.trait_table.min_y, get_grid_width(&g_layout.trait_table),
            TRAIT_TABLE_ROW_0_ID);
        if (g_selected_trait_index < ARRAY_COUNT(g_traits))
        {
            draw_filled_rectangle(g_window_horizontal_scroll.viewport_extent_along_length,
                g_window_horizontal_scroll.viewport_extent_along_thickness,
                g_layout.trait_table.column_min_x_values[1],
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
                if (do_button_action(g_window_horizontal_scroll.viewport_extent_along_length,
                    g_window_horizontal_scroll.viewport_extent_along_thickness,
                    g_allocate_button_min_x, g_allocate_button_min_y, g_table_row_height,
                    g_table_row_height, ALLOCATE_BUTTON_ID))
                {
                    ++*selected_trait_dice;
                    --g_unallocated_dice[g_selected_die_denomination_index];
                }
                allocate_button_border_color = g_black;
            }
            if (*selected_trait_dice)
            {
                if (do_button_action(g_window_horizontal_scroll.viewport_extent_along_length,
                    g_window_horizontal_scroll.viewport_extent_along_thickness,
                    g_deallocate_button_min_x, g_deallocate_button_min_y, g_table_row_height,
                    g_table_row_height, DEALLOCATE_BUTTON_ID))
                {
                    --*selected_trait_dice;
                    ++g_unallocated_dice[g_selected_die_denomination_index];
                }
                deallocate_button_border_color = g_black;
            }
        }
        draw_grid(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness,
            &g_layout.unallocated_dice_table);
        int32_t cell_x = g_layout.unallocated_dice_table.column_min_x_values[0] + g_line_thickness;
        int32_t text_y = g_skill_table_frame_min_y - g_text_padding;
        uint32_t unallocated_dice_cells_width =
            get_grid_width(&g_layout.unallocated_dice_table) - g_line_thickness;
        draw_horizontally_centered_string("Unallocated Dice",
            g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness, cell_x, text_y,
            unallocated_dice_cells_width);
        text_y += g_table_row_height;
        for (size_t i = 0; i < g_layout.unallocated_dice_table.column_count; ++i)
        {
            draw_horizontally_centered_string(g_die_denominations[i],
                g_window_horizontal_scroll.viewport_extent_along_length,
                g_window_horizontal_scroll.viewport_extent_along_thickness, cell_x, text_y,
                die_cell_width);
            draw_uint8(g_window_horizontal_scroll.viewport_extent_along_length,
                g_window_horizontal_scroll.viewport_extent_along_thickness, g_unallocated_dice[i],
                cell_x + g_text_padding, text_y + g_table_row_height);
            cell_x += g_die_column_width;
        }
        draw_grid(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness, &g_layout.trait_table);
        draw_grid(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness,
            &(Grid) { 3, 1, g_layout.unallocated_dice_table.column_min_x_values,
                g_layout.trait_table.min_y - g_table_row_height });
        text_y = g_layout.trait_table.min_y - g_text_padding;
        draw_horizontally_centered_string("Traits",
            g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness,
            g_layout.trait_table.column_min_x_values[0], text_y,
            g_trait_column_width + g_line_thickness);
        draw_horizontally_centered_string("Dice",
            g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness,
            g_layout.unallocated_dice_table.column_min_x_values[0] + g_line_thickness,
            text_y - g_table_row_height, unallocated_dice_cells_width);
        for (size_t i = 0; i < 3; ++i)
        {
            draw_horizontally_centered_string(g_die_denominations[i],
                g_window_horizontal_scroll.viewport_extent_along_length,
                g_window_horizontal_scroll.viewport_extent_along_thickness,
                g_layout.unallocated_dice_table.column_min_x_values[i] + g_line_thickness, text_y,
                die_cell_width);
        }
        for (size_t i = 0; i < g_layout.trait_table.row_count; ++i)
        {
            text_y += g_table_row_height;
            Trait*trait = g_traits + i;
            draw_string(trait->name, g_window_horizontal_scroll.viewport_extent_along_length,
                g_window_horizontal_scroll.viewport_extent_along_thickness,
                g_layout.trait_table.column_min_x_values[0] + g_line_thickness + g_text_padding,
                text_y);
            for (size_t die_index = 0; die_index < ARRAY_COUNT(trait->dice); ++die_index)
            {
                draw_uint8(g_window_horizontal_scroll.viewport_extent_along_length,
                    g_window_horizontal_scroll.viewport_extent_along_thickness,
                    trait->dice[die_index],
                    g_layout.unallocated_dice_table.column_min_x_values[die_index] +
                        g_line_thickness + g_text_padding, text_y);
            }
        }
    }
    else
    {
        select_table_row(clipped_skill_table_viewport_width, clipped_skill_table_viewport_height,
            &g_selected_skill_index, ARRAY_COUNT(g_skills), g_skill_table.column_min_x_values[0],
            g_skill_table.min_y, skill_table_width, SKILL_TABLE_ROW_0_ID);
        if (g_selected_skill_index < ARRAY_COUNT(g_skills))
        {
            draw_filled_rectangle(clipped_skill_table_viewport_width,
                clipped_skill_table_viewport_height,
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
                if (do_button_action(g_window_horizontal_scroll.viewport_extent_along_length,
                    g_window_horizontal_scroll.viewport_extent_along_thickness,
                    g_allocate_button_min_x, g_allocate_button_min_y, g_table_row_height,
                    g_table_row_height, ALLOCATE_BUTTON_ID))
                {
                    ++selected_skill->mark_count;
                    --g_unallocated_marks;
                }
                allocate_button_border_color = g_black;
            }
            if (selected_skill->mark_count)
            {
                if (do_button_action(g_window_horizontal_scroll.viewport_extent_along_length,
                    g_window_horizontal_scroll.viewport_extent_along_thickness,
                    g_deallocate_button_min_x, g_deallocate_button_min_y, g_table_row_height,
                    g_table_row_height, DEALLOCATE_BUTTON_ID))
                {
                    --selected_skill->mark_count;
                    ++g_unallocated_marks;
                }
                deallocate_button_border_color = g_black;
            }
        }
        draw_rectangle_outline(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness,
            g_layout.unallocated_marks_display_min_x, g_layout.unallocated_marks_display_min_y,
            g_layout.unallocated_marks_display_width, g_table_row_height, g_black);
        int32_t text_x =
            g_layout.unallocated_marks_display_min_x + g_line_thickness + g_text_padding;
        int32_t text_y = g_deallocate_button_min_y - g_text_padding;
        draw_string("Unallocated Marks", g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness, text_x, text_y);
        draw_uint8(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness, g_unallocated_marks, text_x,
            text_y + g_table_row_height);
    }
    draw_grid(g_window_horizontal_scroll.viewport_extent_along_length,
        g_window_horizontal_scroll.viewport_extent_along_thickness, &g_skill_table_dice_header);
    draw_horizontally_centered_string("Dice",
        g_window_horizontal_scroll.viewport_extent_along_length,
        g_window_horizontal_scroll.viewport_extent_along_thickness,
        g_skill_table.column_min_x_values[2] + g_line_thickness,
        g_skill_table_dice_header.min_y - g_text_padding,
        get_grid_width(&g_skill_table_dice_header) - g_line_thickness);
    uint32_t die_column_width_with_both_borders = g_die_column_width + g_line_thickness;
    int32_t text_y = g_skill_table_frame_min_y - g_text_padding;
    for (size_t i = 2; i < g_skill_table.column_count; ++i)
    {
        draw_horizontally_centered_string(g_die_denominations[i - 2],
            g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness,
            g_skill_table.column_min_x_values[i], text_y, die_column_width_with_both_borders);
    }
    draw_grid_dividers(clipped_skill_table_viewport_width, clipped_skill_table_viewport_height,
        &g_skill_table);
    draw_rectangle_outline(g_window_horizontal_scroll.viewport_extent_along_length,
        g_window_horizontal_scroll.viewport_extent_along_thickness,
        g_skill_table.column_min_x_values[0], g_skill_table_frame_min_y, skill_table_width,
        SKILL_TABLE_SCROLL.viewport_extent_along_length.length + g_line_thickness, g_black);
    draw_horizontally_centered_string("Skills",
        g_window_horizontal_scroll.viewport_extent_along_length,
        g_window_horizontal_scroll.viewport_extent_along_thickness,
        g_skill_table.column_min_x_values[0], text_y,
        g_skill_table.column_min_x_values[1] + g_line_thickness -
            g_skill_table.column_min_x_values[0]);
    draw_string("Marks", g_window_horizontal_scroll.viewport_extent_along_length,
        g_window_horizontal_scroll.viewport_extent_along_thickness,
        g_skill_table.column_min_x_values[1] + g_line_thickness + g_text_padding, text_y);
    int32_t skill_text_x = g_skill_table.column_min_x_values[0] + g_line_thickness + g_text_padding;
    text_y = g_skill_table.min_y - g_text_padding;
    for (size_t i = 0; i < ARRAY_COUNT(g_skills); ++i)
    {
        text_y += g_table_row_height;
        Skill*skill = g_skills + i;
        draw_string(skill->name, clipped_skill_table_viewport_width,
            clipped_skill_table_viewport_height, skill_text_x, text_y);
        draw_uint8(clipped_skill_table_viewport_width, clipped_skill_table_viewport_height,
            skill->mark_count,
            g_skill_table.column_min_x_values[1] + g_line_thickness + g_text_padding, text_y);
        uint8_t die_counts[ARRAY_COUNT(g_die_denominations)] = { 0 };
        if (skill->mark_count)
        {
            ++die_counts[skill->mark_count - 1];
        }
        for (size_t die_index = 0; die_index < ARRAY_COUNT(g_die_denominations); ++die_index)
        {
            draw_uint8(clipped_skill_table_viewport_width, clipped_skill_table_viewport_height,
                die_counts[die_index],
                g_skill_table.column_min_x_values[2 + die_index] + g_line_thickness +
                    g_text_padding,
                text_y);
        }
    }
    if (allocate_button_border_color.value == g_black.value)
    {
        draw_rectangle_outline(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness, g_deallocate_button_min_x,
            g_deallocate_button_min_y, g_table_row_height, g_table_row_height,
            deallocate_button_border_color);
        draw_rectangle_outline(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness, g_allocate_button_min_x,
            g_allocate_button_min_y, g_table_row_height, g_table_row_height,
            allocate_button_border_color);
    }
    else
    {
        draw_rectangle_outline(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness, g_allocate_button_min_x,
            g_allocate_button_min_y, g_table_row_height, g_table_row_height,
            allocate_button_border_color);
        draw_rectangle_outline(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness, g_deallocate_button_min_x,
            g_deallocate_button_min_y, g_table_row_height, g_table_row_height,
            deallocate_button_border_color);
    }
    draw_rasterization(g_window_horizontal_scroll.viewport_extent_along_length,
        g_window_horizontal_scroll.viewport_extent_along_thickness, g_deallocate_button_arrow,
        g_deallocate_button_min_x + (g_table_row_height + g_line_thickness -
            g_right_arrowhead_rasterization.bitmap.width) / 2,
        g_deallocate_button_min_y + (g_table_row_height + g_line_thickness -
            g_right_arrowhead_rasterization.bitmap.rows) / 2, deallocate_button_border_color);
    draw_rasterization(g_window_horizontal_scroll.viewport_extent_along_length,
        g_window_horizontal_scroll.viewport_extent_along_thickness, g_allocate_button_arrow,
        g_allocate_button_min_x + (g_table_row_height + g_line_thickness -
            g_left_arrowhead_rasterization.bitmap.width) / 2,
        g_allocate_button_min_y + (g_table_row_height + g_line_thickness -
            g_left_arrowhead_rasterization.bitmap.rows) / 2, allocate_button_border_color);
    g_left_mouse_button_changed_state = false;
    g_120ths_of_mouse_wheel_notches_turned = 0;
    if (!g_left_mouse_button_is_down)
    {
        g_clicked_control_id = 0;
    }
}