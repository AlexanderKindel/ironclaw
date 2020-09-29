#include <windows.h>
#include <windowsx.h>

size_t copy_font_data_to_stack_cursor(HWND window_handle, WORD dpi);

#define SET_MESSAGE_FONT_SIZE(dpi)\
{\
    NONCLIENTMETRICSW non_client_metrics;\
    non_client_metrics.cbSize = sizeof(NONCLIENTMETRICSW);\
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, non_client_metrics.cbSize, &non_client_metrics,\
        0);\
    g_message_font_height = ((-non_client_metrics.lfMessageFont.lfHeight * 72) << 6) / dpi;\
}

#define COPY_FONT_DATA_TO_STACK_CURSOR(param, dpi) copy_font_data_to_stack_cursor(param, dpi)

#define SET_INITIAL_CURSOR_POSITION()\
{\
    POINT cursor_position;\
    GetCursorPos(&cursor_position);\
    g_cursor_x = cursor_position.x;\
    g_cursor_y = cursor_position.y;\
}

#define SET_PAGE_SIZE()\
{\
    SYSTEM_INFO system_info;\
    GetSystemInfo(&system_info);\
    g_page_size = max(system_info.dwAllocationGranularity, system_info.dwPageSize);\
}

#define RESERVE_MEMORY(size) VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE)

#define COMMIT_PAGE(address) VirtualAlloc(address, g_page_size, MEM_COMMIT, PAGE_READWRITE)

#include "create_character_window.c"

size_t copy_font_data_to_stack_cursor(HWND window_handle, WORD dpi)
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
    return font_file_size;
}

HDC g_device_context;
BITMAPINFO g_pixel_buffer_info;

LRESULT CALLBACK create_character_proc(HWND window_handle, UINT message, WPARAM w_param,
    LPARAM l_param)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_DPICHANGED:
        FT_Done_Face(g_face);
        WORD dpi = HIWORD(w_param);
        g_stack.cursor = g_stack.start;
        scale_graphics_to_dpi(g_stack.start, copy_font_data_to_stack_cursor(window_handle, dpi),
            dpi);
        reset_widget_positions();
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
    case WM_MOUSEWHEEL:
        g_120ths_of_mouse_wheel_notches_turned = GET_WHEEL_DELTA_WPARAM(w_param);
        return 0;
    case WM_SIZE:
        g_window_x_extent.length = LOWORD(l_param);
        g_window_y_extent.length = HIWORD(l_param);
        g_pixel_buffer_info.bmiHeader.biWidth = g_window_x_extent.length;
        g_pixel_buffer_info.bmiHeader.biHeight = -(int32_t)g_window_y_extent.length;
        size_t new_pixel_buffer_capacity =
            GetSystemMetrics(SM_CXVIRTUALSCREEN) * GetSystemMetrics(SM_CXVIRTUALSCREEN);
        if (new_pixel_buffer_capacity > g_pixel_buffer_capacity)
        {
            VirtualFree(g_pixels, 0, MEM_RELEASE);
            g_pixels = VirtualAlloc(0, sizeof(Color) * new_pixel_buffer_capacity, MEM_COMMIT,
                PAGE_READWRITE);
        }
        reformat_widgets_on_window_resize();
        handle_message();
        StretchDIBits(g_device_context, 0, 0, g_window_x_extent.length, g_window_y_extent.length, 0,
            0, g_window_x_extent.length, g_window_y_extent.length, g_pixels, &g_pixel_buffer_info,
            DIB_RGB_COLORS, SRCCOPY);
        return 0;
    case WM_KEYDOWN:
        if (w_param = VK_SHIFT)
        {
            g_shift_is_down = true;
            return 0;
        }
        break;
    case WM_KEYUP:
        if (w_param = VK_SHIFT)
        {
            g_shift_is_down = false;
            return 0;
        }
    }
    return DefWindowProc(window_handle, message, w_param, l_param);
}

HWND init(HINSTANCE instance_handle)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &g_text_lines_per_mouse_wheel_notch, 0);
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
    WORD dpi = GetDpiForWindow(window_handle);
    INIT(window_handle, dpi);
    ShowWindow(window_handle, SW_MAXIMIZE);
    return window_handle;
}

int WINAPI wWinMain(HINSTANCE instance_handle, HINSTANCE previous_instance_handle,
    PWSTR command_line, int show)
{
    g_device_context = GetDC(init(instance_handle));
    MSG msg;
    while (GetMessage(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        handle_message();
        StretchDIBits(g_device_context, 0, 0, g_window_x_extent.length, g_window_y_extent.length, 0,
            0, g_window_x_extent.length, g_window_y_extent.length, g_pixels, &g_pixel_buffer_info,
            DIB_RGB_COLORS, SRCCOPY);
    }
    return 0;
}