using System;
using System.Runtime.InteropServices;
using System.Text;

namespace WinUIShell;

[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
internal struct SB_DesktopIcon
{
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
    public string name;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
    public string path;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
    public string parsingName;
    public int x;
    public int y;
}

[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
internal struct SB_AppItem
{
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
    public string name;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
    public string path;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
    public string target;
    public int type;
}

[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
internal struct SHFILEINFOW
{
    public IntPtr hIcon;
    public int iIcon;
    public uint dwAttributes;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
    public string szDisplayName;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 80)]
    public string szTypeName;
}

[StructLayout(LayoutKind.Sequential)]
internal struct APPBARDATA
{
    public int cbSize;
    public IntPtr hWnd;
    public uint uCallbackMessage;
    public uint uEdge;
    public RECT rc;
    public int lParam;
}

[StructLayout(LayoutKind.Sequential)]
internal struct RECT { public int left, top, right, bottom; }

internal static class NativeMethods
{
    const string DLL = "shell_backend.dll";

    [DllImport(DLL)] internal static extern void SB_Power_Lock();
    [DllImport(DLL)] internal static extern void SB_Power_Sleep();
    [DllImport(DLL)] internal static extern void SB_Power_Restart();
    [DllImport(DLL)] internal static extern void SB_Power_Shutdown();

    [DllImport(DLL)] internal static extern int SB_Volume_Get();
    [DllImport(DLL)] internal static extern void SB_Volume_Set(int val);

    [DllImport(DLL)] internal static extern bool SB_Brightness_HasControl();
    [DllImport(DLL)] internal static extern int SB_Brightness_Get();
    [DllImport(DLL)] internal static extern void SB_Brightness_Set(int val);

    [DllImport(DLL, CharSet = CharSet.Unicode)]
    internal static extern bool SB_Network_GetInfo(StringBuilder buf, int bufLen);

    [DllImport(DLL)] internal static extern void SB_WiFi_Toggle();
    [DllImport(DLL)] internal static extern bool SB_WiFi_IsEnabled();
    [DllImport(DLL)] internal static extern void SB_Bluetooth_Toggle();
    [DllImport(DLL)] internal static extern bool SB_Bluetooth_IsEnabled();
    [DllImport(DLL)] internal static extern bool SB_Bluetooth_HasHardware();

    [DllImport(DLL)]
    internal static extern int SB_Desktop_EnumerateIcons([Out] SB_DesktopIcon[]? icons, int maxIcons);

    [DllImport(DLL, CharSet = CharSet.Unicode)]
    internal static extern int SB_Launcher_Enumerate([Out] SB_AppItem[] items, int maxItems);
    [DllImport(DLL, CharSet = CharSet.Unicode)]
    internal static extern void SB_Launcher_Launch(string path);

    [DllImport(DLL, CharSet = CharSet.Unicode)]
    internal static extern bool SB_Config_GetWallpaperPath(StringBuilder buf, int bufLen);

    // Win32
    [DllImport("user32.dll")]
    internal static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter,
        int x, int y, int cx, int cy, uint uFlags);

    [DllImport("user32.dll")]
    internal static extern int GetWindowLongW(IntPtr hWnd, int nIndex);

    [DllImport("user32.dll")]
    internal static extern int SetWindowLongW(IntPtr hWnd, int nIndex, int dwNewLong);

    [DllImport("shell32.dll")]
    internal static extern IntPtr SHAppBarMessage(int dwMessage, ref APPBARDATA pData);

    [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
    internal static extern IntPtr SHGetFileInfoW(string pszPath, uint dwFileAttributes,
        out SHFILEINFOW psfi, int cbFileInfo, uint uFlags);

    [DllImport("user32.dll", SetLastError = true)]
    internal static extern bool DestroyIcon(IntPtr hIcon);

    internal static readonly IntPtr HWND_BOTTOM = new IntPtr(1);
    internal static readonly IntPtr HWND_TOPMOST = new IntPtr(-1);

    internal const uint SWP_SHOWWINDOW = 0x0040;
    internal const uint SWP_NOACTIVATE = 0x0010;
    internal const uint SWP_FRAMECHANGED = 0x0020;

    internal const int GWL_EXSTYLE = -20;
    internal const int GWL_STYLE = -16;

internal const int WS_EX_TOOLWINDOW = 0x00000080;
internal const int WS_EX_TOPMOST = 0x00000008;
internal const int WS_EX_NOACTIVATE = 0x08000000;
internal const uint WS_POPUP = 0x80000000;
internal const uint WS_VISIBLE = 0x10000000;

    internal const uint SHGFI_ICON = 0x000000100;
    internal const uint SHGFI_LARGEICON = 0x000000000;
    internal const uint SHGFI_USEFILEATTRIBUTES = 0x000000010;

    internal const int ABM_NEW = 0;
    internal const int ABM_SETPOS = 3;
    internal const int ABE_BOTTOM = 3;
}
