#include "ironclaw.c"

#define TABS(macro)\
macro(TAB_ALLOCATE_DICE, ("Allocate Trait Dice"))\
macro(TAB_ALLOCATE_MARKS, ("Allocate Marks"))\
macro(TAB_CHOOSE_GIFTS, ("Choose Gifts"))

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
    SKILL_TABLE_ROW_0_ID,
    MAX_WINDOW_CONTROL_ID
};

ScrollBar g_window_horizontal_scroll;
ScrollBar g_vertical_scrolls[3];

#define WINDOW_VERTICAL_SCROLL g_vertical_scrolls[0]
#define SKILL_TABLE_SCROLL g_vertical_scrolls[1]
#define GIFT_LIST_SCROLL g_vertical_scrolls[2]

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

    Grid unallocated_dice_table;
    struct
    {
        int32_t unallocated_marks_display_min_x;
        int32_t unallocated_marks_display_min_y;
        uint32_t unallocated_marks_display_width;
    };
    struct
    {
        Grid species_gifts;
        Grid gift_list;
        int32_t gift_list_frame_min_y;
        int32_t species_gift_column_min_x_values[2];
        int32_t gift_list_column_min_x_values[2];
    };
} TabLayout;

TabLayout g_layout;

enum AllocateTraitDiceTabID
{
    UNALLOCATED_DICE_TABLE_COLUMN_0_ID = SKILL_TABLE_ROW_0_ID,
    TRAIT_TABLE_ROW_0_ID = UNALLOCATED_DICE_TABLE_COLUMN_0_ID + ARRAY_COUNT(g_unallocated_dice)
};

int32_t g_trait_table_column_min_x_values[2 + ARRAY_COUNT(g_unallocated_dice)];
Grid g_trait_table;
size_t g_selected_die_denomination_index = ARRAY_COUNT(g_unallocated_dice);
size_t g_selected_trait_index = ARRAY_COUNT(g_traits);
uint32_t g_trait_column_width;

#define UNALLOCATED_MARKS_ID MAX_WINDOW_CONTROL_ID

size_t g_selected_skill_index = ARRAY_COUNT(g_skills);
uint8_t g_unallocated_marks = 13;

enum ChooseGiftsTabID
{
    GIFT_LIST_SCROLL_ID = MAX_WINDOW_CONTROL_ID,
    GIFT_LIST_ROW_0_ID
};

#define GIFT_LIST_ROW_0_ID MAX_WINDOW_CONTROL_ID

uint8_t g_free_gifts_chosen_count;
uint8_t g_free_gift_indices[3];
uint32_t g_gift_list_width;
size_t g_selected_gift_index = ARRAY_COUNT(g_gifts);

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
    GIFT_LIST_SCROLL.content_length = g_table_row_height * ARRAY_COUNT(g_gifts) - g_line_thickness;
    g_gift_list_width = 0;
    for (size_t i = 0; i < ARRAY_COUNT(g_gifts); ++i)
    {
        g_gift_list_width = max32(g_gift_list_width, get_string_width(g_gifts[i].name));
    }
    g_gift_list_width += g_line_thickness + 2 * g_text_padding;
    g_tab_width = 0;
    for (size_t i = 0; i < ARRAY_COUNT(g_tab_names); ++i)
    {
        g_tab_width = max32(g_tab_width, get_string_width(g_tab_names[i]));
    }
    g_tab_width += g_line_thickness + 2 * g_text_padding;
}

void reset_widget_x_positions(void)
{
    uint32_t cell_height = g_table_row_height - g_line_thickness;
    switch (g_selected_tab_index)
    {
    case TAB_ALLOCATE_DICE:
    {
        g_trait_table.column_min_x_values[0] =
            cell_height - g_window_horizontal_scroll.content_offset;
        if (WINDOW_VERTICAL_SCROLL.is_active)
        {
            g_trait_table.column_min_x_values[0] += cell_height;
        }
        g_trait_table.column_min_x_values[1] =
            g_trait_table.column_min_x_values[0] + g_trait_column_width;
        for (size_t i = 2; i <= g_trait_table.column_count; ++i)
        {
            g_trait_table.column_min_x_values[i] =
                g_trait_table.column_min_x_values[i - 1] + g_die_column_width;
        }
        g_deallocate_button_min_x = g_layout.unallocated_dice_table.column_min_x_values[0] +
            (get_grid_width(&g_layout.unallocated_dice_table) + g_line_thickness) / 2;
        g_allocate_button_min_x = g_deallocate_button_min_x - g_table_row_height;
        g_skill_table.column_min_x_values[0] =
            get_rightmost_column_x(&g_trait_table) + g_table_row_height;
        break;
    }
    case TAB_ALLOCATE_MARKS:
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
        break;
    }
    case TAB_CHOOSE_GIFTS:
    {
        g_layout.species_gifts.column_min_x_values[0] =
            cell_height - g_window_horizontal_scroll.content_offset;
        if (WINDOW_VERTICAL_SCROLL.is_active)
        {
            g_layout.species_gifts.column_min_x_values[0] += cell_height;
        }
        g_layout.species_gifts.column_min_x_values[1] =
            g_layout.species_gifts.column_min_x_values[0] + g_gift_list_width;
        g_allocate_button_min_x =
            g_layout.species_gifts.column_min_x_values[1] + g_table_row_height;
        g_deallocate_button_min_x = g_allocate_button_min_x;
        g_layout.gift_list_column_min_x_values[0] =
            g_deallocate_button_min_x + 2 * g_table_row_height;
        if (GIFT_LIST_SCROLL.is_active)
        {
            GIFT_LIST_SCROLL.min_along_thickness = g_layout.gift_list_column_min_x_values[0];
            g_layout.gift_list_column_min_x_values[0] += cell_height;
        }
        GIFT_LIST_SCROLL.viewport_extent_along_thickness.min =
            g_layout.gift_list_column_min_x_values[0] + g_line_thickness;
        g_layout.gift_list_column_min_x_values[1] =
            g_layout.gift_list_column_min_x_values[0] + g_gift_list_width;
        g_skill_table.column_min_x_values[0] =
            g_layout.gift_list_column_min_x_values[1] + g_table_row_height;
    }
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
    uint32_t cell_height = g_table_row_height - g_line_thickness;
    g_skill_table_dice_header.min_y = cell_height - WINDOW_VERTICAL_SCROLL.content_offset;
    g_skill_table_frame_min_y = g_skill_table_dice_header.min_y + g_table_row_height;
    if (SKILL_TABLE_SCROLL.is_active)
    {
        SKILL_TABLE_SCROLL.min_along_length = g_skill_table_frame_min_y;
    }
    SKILL_TABLE_SCROLL.viewport_extent_along_length.min =
        g_skill_table_frame_min_y + g_line_thickness;
    g_skill_table.min_y = g_skill_table_frame_min_y - SKILL_TABLE_SCROLL.content_offset;
    switch (g_selected_tab_index)
    {
    case TAB_ALLOCATE_DICE:
    {
        g_layout.unallocated_dice_table.min_y = g_skill_table_frame_min_y;
        g_allocate_button_min_y = g_skill_table_frame_min_y + 3 * g_table_row_height;
        g_deallocate_button_min_y = g_allocate_button_min_y;
        g_trait_table.min_y = g_allocate_button_min_y + 3 * g_table_row_height;
        break;
    }
    case TAB_ALLOCATE_MARKS:
    {
        g_allocate_button_min_y = g_skill_table_frame_min_y;
        g_deallocate_button_min_y = g_allocate_button_min_y + g_table_row_height;
        g_layout.unallocated_marks_display_min_y = g_deallocate_button_min_y;
        break;
    }
    case TAB_CHOOSE_GIFTS:
    {
        g_allocate_button_min_y =
            g_skill_table_dice_header.min_y + 8 * g_table_row_height + g_table_row_height / 2;
        g_deallocate_button_min_y = g_allocate_button_min_y + g_table_row_height;
        g_layout.species_gifts.min_y = g_skill_table_dice_header.min_y;
        g_layout.gift_list_frame_min_y = g_skill_table_dice_header.min_y;
        g_layout.gift_list.min_y = g_layout.gift_list_frame_min_y - GIFT_LIST_SCROLL.content_offset;
        GIFT_LIST_SCROLL.min_along_length = g_layout.gift_list_frame_min_y;
        GIFT_LIST_SCROLL.viewport_extent_along_length.min =
            GIFT_LIST_SCROLL.min_along_length + g_line_thickness;
    }
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
    size_t vertical_scroll_count;
    uint32_t window_scroll_viewport_heights_at_which_vertical_scrolls_activate
        [ARRAY_COUNT(g_vertical_scrolls)];
    size_t scroll_activation_order[ARRAY_COUNT(g_vertical_scrolls)];
    int32_t unscrolled_skill_table_frame_min_y = cell_height + g_table_row_height;
    window_scroll_viewport_heights_at_which_vertical_scrolls_activate[1] =
        unscrolled_skill_table_frame_min_y + (1 + g_skill_table.row_count) * g_table_row_height;
    switch (g_selected_tab_index)
    {
    case TAB_ALLOCATE_DICE:
    {
        vertical_scroll_count = 2;
        g_window_horizontal_scroll.content_length = get_grid_width(&g_trait_table) +
            get_grid_width(&g_skill_table) + 3 * g_table_row_height + g_tab_width;
        scroll_activation_order[0] = 1;
        scroll_activation_order[1] = 0;
        WINDOW_VERTICAL_SCROLL.content_length =
            cell_height + (g_trait_table.row_count + 8) * g_table_row_height;
        break;
    }
    case TAB_ALLOCATE_MARKS:
    {
        vertical_scroll_count = 2;
        g_window_horizontal_scroll.content_length = g_layout.unallocated_marks_display_width +
            5 * g_table_row_height + get_grid_width(&g_skill_table) + g_tab_width;
        scroll_activation_order[0] = 1;
        scroll_activation_order[1] = 0;
        WINDOW_VERTICAL_SCROLL.content_length = cell_height + 4 * g_table_row_height;
        break;
    }
    case TAB_CHOOSE_GIFTS:
    {
        vertical_scroll_count = 3;
        g_window_horizontal_scroll.content_length = 6 * g_table_row_height + 2 * g_gift_list_width +
            get_grid_width(&g_skill_table) + g_tab_width;
        scroll_activation_order[0] = 2;
        scroll_activation_order[1] = 1;
        scroll_activation_order[2] = 0;
        window_scroll_viewport_heights_at_which_vertical_scrolls_activate[2] =
            cell_height + g_table_row_height * (ARRAY_COUNT(g_gifts) + 1);
        WINDOW_VERTICAL_SCROLL.content_length = cell_height + 12 * g_table_row_height;
    }
    }
    window_scroll_viewport_heights_at_which_vertical_scrolls_activate[0] =
        WINDOW_VERTICAL_SCROLL.content_length;
    for (size_t i = 0; i < vertical_scroll_count; ++i)
    {
        g_vertical_scrolls[i].is_active = false;
    }
    g_window_horizontal_scroll.is_active = false;
    uint32_t window_content_width = g_window_horizontal_scroll.content_length;
    uint32_t window_scroll_viewport_height = g_window_y_extent.length;
    size_t scroll_activation_index = 0;
    while (true)
    {
        while (true)
        {
            if (scroll_activation_index == vertical_scroll_count)
            {
                if (window_content_width > g_window_x_extent.length)
                {
                    g_window_horizontal_scroll.is_active = true;
                }
                goto scroll_activity_states_are_set;
            }
            if (window_scroll_viewport_heights_at_which_vertical_scrolls_activate
                [scroll_activation_order[scroll_activation_index]] > window_scroll_viewport_height)
            {
                g_vertical_scrolls[scroll_activation_order[scroll_activation_index]].is_active =
                    true;
                window_content_width += cell_height;
                ++scroll_activation_index;
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
            ++scroll_activation_index;
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
    GIFT_LIST_SCROLL.viewport_extent_along_thickness.length = g_gift_list_width - g_line_thickness;
    GIFT_LIST_SCROLL.viewport_extent_along_length.length =
        max32(g_window_horizontal_scroll.viewport_extent_along_thickness.length -
            g_table_row_height, WINDOW_VERTICAL_SCROLL.content_length - g_table_row_height) -
        g_table_row_height;
    if (GIFT_LIST_SCROLL.is_active)
    {
        g_window_horizontal_scroll.content_length += cell_height;
        GIFT_LIST_SCROLL.length =
            GIFT_LIST_SCROLL.viewport_extent_along_length.length + 2 * g_line_thickness;
    }
    else
    {
        GIFT_LIST_SCROLL.content_offset = 0;
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
    g_layout.unallocated_dice_table.column_min_x_values = g_trait_table_column_min_x_values + 1;
    g_trait_table.column_count = ARRAY_COUNT(g_trait_table_column_min_x_values) - 1;
    g_trait_table.row_count = ARRAY_COUNT(g_traits);
    g_trait_table.column_min_x_values = g_trait_table_column_min_x_values;
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
    if (g_selected_tab_index == TAB_CHOOSE_GIFTS &&
        update_vertical_scroll_bar_offset(&GIFT_LIST_SCROLL, GIFT_LIST_SCROLL_ID))
    {
        g_layout.gift_list.min_y = g_layout.gift_list_frame_min_y - GIFT_LIST_SCROLL.content_offset;
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
    if (g_selected_tab_index == TAB_CHOOSE_GIFTS)
    {
        draw_vertical_scroll_bar(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness, &GIFT_LIST_SCROLL);
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
        switch (g_selected_tab_index)
        {
        case TAB_ALLOCATE_DICE:
        {
            select_allocate_trait_dice_tab();
            break;
        }
        case TAB_ALLOCATE_MARKS:
        {
            g_allocate_button_arrow = &g_right_arrowhead_rasterization;
            g_deallocate_button_arrow = &g_left_arrowhead_rasterization;
            g_layout.unallocated_marks_display_width =
                g_line_thickness + get_string_width("Unallocated Marks") + 2 * g_text_padding;
            reformat_widgets_on_window_resize();
            break;
        }
        case TAB_CHOOSE_GIFTS:
        {
            g_allocate_button_arrow = &g_left_arrowhead_rasterization;
            g_deallocate_button_arrow = &g_right_arrowhead_rasterization;
            g_layout.species_gifts.row_count = 3;
            g_layout.species_gifts.column_count = 1;
            g_layout.species_gifts.column_min_x_values = g_layout.species_gift_column_min_x_values;
            g_layout.gift_list.row_count = ARRAY_COUNT(g_gifts);
            g_layout.gift_list.column_count = 1;
            g_layout.gift_list.column_min_x_values = g_layout.gift_list_column_min_x_values;
            reformat_widgets_on_window_resize();
        }
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
    Interval clipped_skill_table_viewport_x_extent;
    Interval clipped_skill_table_viewport_y_extent;
    intersect_viewports(g_window_horizontal_scroll.viewport_extent_along_length,
        g_window_horizontal_scroll.viewport_extent_along_thickness,
        SKILL_TABLE_SCROLL.viewport_extent_along_thickness,
        SKILL_TABLE_SCROLL.viewport_extent_along_length, &clipped_skill_table_viewport_x_extent,
        &clipped_skill_table_viewport_y_extent);
    switch (g_selected_tab_index)
    {
    case TAB_ALLOCATE_DICE:
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
            ARRAY_COUNT(g_traits), g_trait_table.column_min_x_values[0], g_trait_table.min_y,
            get_grid_width(&g_trait_table), TRAIT_TABLE_ROW_0_ID);
        if (g_selected_trait_index < ARRAY_COUNT(g_traits))
        {
            draw_filled_rectangle(g_window_horizontal_scroll.viewport_extent_along_length,
                g_window_horizontal_scroll.viewport_extent_along_thickness,
                g_trait_table.column_min_x_values[1],
                g_line_thickness + g_trait_table.min_y +
                    g_selected_trait_index * g_table_row_height,
                g_trait_table.column_min_x_values[g_trait_table.column_count] -
                    g_trait_table.column_min_x_values[1],
                cell_height, g_white);
            if (g_selected_die_denomination_index < 3)
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
            g_window_horizontal_scroll.viewport_extent_along_thickness, &g_trait_table);
        draw_grid(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness,
            &(Grid) { 3, 1, g_layout.unallocated_dice_table.column_min_x_values,
            g_trait_table.min_y - g_table_row_height });
        text_y = g_trait_table.min_y - g_text_padding;
        draw_horizontally_centered_string("Traits",
            g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness,
            g_trait_table.column_min_x_values[0], text_y, g_trait_column_width + g_line_thickness);
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
        for (size_t i = 0; i < g_trait_table.row_count; ++i)
        {
            text_y += g_table_row_height;
            Trait*trait = g_traits + i;
            draw_string(trait->name, g_window_horizontal_scroll.viewport_extent_along_length,
                g_window_horizontal_scroll.viewport_extent_along_thickness,
                g_trait_table.column_min_x_values[0] + g_line_thickness + g_text_padding, text_y);
            for (size_t die_index = 0; die_index < ARRAY_COUNT(trait->dice); ++die_index)
            {
                draw_uint8(g_window_horizontal_scroll.viewport_extent_along_length,
                    g_window_horizontal_scroll.viewport_extent_along_thickness,
                    trait->dice[die_index],
                    g_layout.unallocated_dice_table.column_min_x_values[die_index] +
                    g_line_thickness + g_text_padding, text_y);
            }
        }
        break;
    }
    case TAB_ALLOCATE_MARKS:
    {
        select_table_row(clipped_skill_table_viewport_x_extent,
            clipped_skill_table_viewport_y_extent, &g_selected_skill_index, ARRAY_COUNT(g_skills),
            g_skill_table.column_min_x_values[0], g_skill_table.min_y, skill_table_width,
            SKILL_TABLE_ROW_0_ID);
        if (g_selected_skill_index < ARRAY_COUNT(g_skills))
        {
            draw_filled_rectangle(clipped_skill_table_viewport_x_extent,
                clipped_skill_table_viewport_y_extent,
                g_line_thickness + g_skill_table.column_min_x_values[1],
                g_line_thickness + g_skill_table.min_y +
                    g_selected_skill_index * g_table_row_height,
                g_skill_table.column_min_x_values[2] -
                    (g_skill_table.column_min_x_values[1] + g_line_thickness),
                cell_height, g_white);
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
        break;
    }
    case TAB_CHOOSE_GIFTS:
    {
        draw_horizontally_centered_string("Species Gifts",
            g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness,
            g_layout.species_gift_column_min_x_values[0],
            g_layout.species_gifts.min_y - g_text_padding, g_gift_list_width + g_line_thickness);
        draw_grid(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness, &g_layout.species_gifts);
        Grid chosen_gifts = g_layout.species_gifts;
        chosen_gifts.min_y += 4 * g_table_row_height;
        draw_horizontally_centered_string("Career Gifts",
            g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness,
            g_layout.species_gift_column_min_x_values[0], chosen_gifts.min_y - g_text_padding,
            g_gift_list_width + g_line_thickness);
        draw_grid(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness, &chosen_gifts);
        chosen_gifts.min_y += 4 * g_table_row_height;
        int32_t text_y = chosen_gifts.min_y - g_text_padding;
        draw_horizontally_centered_string("Free Gifts",
            g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness,
            g_layout.species_gift_column_min_x_values[0], text_y,
            g_gift_list_width + g_line_thickness);
        Interval clipped_gift_list_viewport_x_extent;
        Interval clipped_gift_list_viewport_y_extent;
        intersect_viewports(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness,
            GIFT_LIST_SCROLL.viewport_extent_along_thickness,
            GIFT_LIST_SCROLL.viewport_extent_along_length, &clipped_gift_list_viewport_x_extent,
            &clipped_gift_list_viewport_y_extent);
        select_table_row(clipped_gift_list_viewport_x_extent, clipped_gift_list_viewport_y_extent,
            &g_selected_gift_index, ARRAY_COUNT(g_gifts), g_layout.gift_list.column_min_x_values[0],
            g_layout.gift_list.min_y, g_gift_list_width, GIFT_LIST_ROW_0_ID);
        if (g_selected_gift_index < ARRAY_COUNT(g_gifts))
        {
            draw_filled_rectangle(clipped_gift_list_viewport_x_extent,
                clipped_gift_list_viewport_y_extent,
                g_line_thickness + g_layout.gift_list.column_min_x_values[0],
                g_line_thickness + g_layout.gift_list.min_y +
                    g_selected_gift_index * g_table_row_height,
                g_gift_list_width - g_line_thickness, cell_height, g_white);
            if (g_free_gifts_chosen_count < ARRAY_COUNT(g_free_gift_indices))
            {
                for (size_t i = 0; i < g_free_gifts_chosen_count; ++i)
                {
                    if (g_free_gift_indices[i] == g_selected_gift_index &&
                        !(g_gifts[g_selected_gift_index].descriptor_flags & DESCRIPTOR_MULTIPLE))
                    {
                        goto requirement_not_satisfied;
                    }
                }
                Gift*selected_gift = g_gifts + g_selected_gift_index;
                for (size_t i = 0; i < selected_gift->requirement_count; ++i)
                {
                    Requirement*requirement = selected_gift->requirements + i;
                    switch (requirement->requirement_type)
                    {
                    case REQUIREMENT_GIFT:
                    {
                        for (size_t j = 0; j < g_free_gifts_chosen_count; ++j)
                        {
                            if (g_free_gift_indices[j] == requirement->gift_index)
                            {
                                goto requirement_satisfied;
                            }
                        }
                        goto requirement_not_satisfied;
                    }
                    case REQUIREMENT_NO_GIFT_WITH_DESCRIPTOR:
                    {
                        for (size_t j = 0; j < g_free_gifts_chosen_count; ++j)
                        {
                            if (g_gifts[g_free_gift_indices[j]].descriptor_flags &
                                requirement->descriptor_flags)
                            {
                                goto requirement_not_satisfied;
                            }
                        }
                        goto requirement_satisfied;
                    }
                    case REQUIREMENT_TRAIT:
                    {
                        Trait*trait = g_traits + requirement->trait_index;
                        for (size_t j = DENOMINATION_D8; j-- > requirement->minimum_denomination;)
                        {
                            if (trait->dice[j] > 0)
                            {
                                goto requirement_satisfied;
                            }
                        }
                        goto requirement_not_satisfied;
                    }
                    }
                requirement_satisfied:;
                }
                if (do_button_action(g_window_horizontal_scroll.viewport_extent_along_length,
                    g_window_horizontal_scroll.viewport_extent_along_thickness,
                    g_allocate_button_min_x, g_allocate_button_min_y, g_table_row_height,
                    g_table_row_height, ALLOCATE_BUTTON_ID))
                {
                    g_free_gift_indices[g_free_gifts_chosen_count] = g_selected_gift_index;
                    ++g_free_gifts_chosen_count;
                }
            requirement_not_satisfied:
                allocate_button_border_color = g_black;
            }
        }
        draw_grid(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness, &chosen_gifts);
        int32_t text_x = chosen_gifts.column_min_x_values[0] + g_line_thickness + g_text_padding;
        for (size_t i = 0; i < g_free_gifts_chosen_count; ++i)
        {
            text_y += g_table_row_height;
            draw_string(g_gifts[g_free_gift_indices[i]].name,
                g_window_horizontal_scroll.viewport_extent_along_length,
                g_window_horizontal_scroll.viewport_extent_along_thickness, text_x, text_y);
        }
        draw_grid_dividers(clipped_gift_list_viewport_x_extent, clipped_gift_list_viewport_y_extent,
            &g_layout.gift_list);
        draw_rectangle_outline(g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness,
            g_layout.gift_list_column_min_x_values[0], g_layout.gift_list_frame_min_y,
            g_gift_list_width,
            GIFT_LIST_SCROLL.viewport_extent_along_length.length + g_line_thickness, g_black);
        draw_horizontally_centered_string("Gifts",
            g_window_horizontal_scroll.viewport_extent_along_length,
            g_window_horizontal_scroll.viewport_extent_along_thickness,
            g_layout.gift_list_column_min_x_values[0],
            g_layout.gift_list_frame_min_y - g_text_padding, g_gift_list_width + g_line_thickness);
        text_x = g_layout.gift_list_column_min_x_values[0] + g_line_thickness + g_text_padding;
        text_y = g_layout.gift_list.min_y - g_text_padding;
        for (size_t i = 0; i < ARRAY_COUNT(g_gifts); ++i)
        {
            text_y += g_table_row_height;
            draw_string(g_gifts[i].name, clipped_gift_list_viewport_x_extent,
                clipped_gift_list_viewport_y_extent, text_x, text_y);
        }
    }
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
    draw_grid_dividers(clipped_skill_table_viewport_x_extent, clipped_skill_table_viewport_y_extent,
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
        draw_string(skill->name, clipped_skill_table_viewport_x_extent,
            clipped_skill_table_viewport_y_extent, skill_text_x, text_y);
        draw_uint8(clipped_skill_table_viewport_x_extent, clipped_skill_table_viewport_y_extent,
            skill->mark_count,
            g_skill_table.column_min_x_values[1] + g_line_thickness + g_text_padding, text_y);
        uint8_t die_counts[ARRAY_COUNT(g_die_denominations)] = { 0 };
        if (skill->mark_count)
        {
            ++die_counts[skill->mark_count - 1];
        }
        for (size_t die_index = 0; die_index < ARRAY_COUNT(g_die_denominations); ++die_index)
        {
            draw_uint8(clipped_skill_table_viewport_x_extent, clipped_skill_table_viewport_y_extent,
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