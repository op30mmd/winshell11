using Microsoft.UI;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Shapes;
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Windows.Graphics;
using Windows.UI;
using WinRT.Interop;

namespace WinUIShell;

public partial class App : Application
{
    private Window? _desktop;
    private Window? _taskbar;
    private Window? _flyout;
    private Window? _startMenu;
    private FlyoutControl? _flyoutControl;
    private StartMenuControl? _startMenuControl;
    private TextBlock? _clockBlock;

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        CreateDesktop();
        CreateFlyout();
        CreateStartMenu();
        CreateTaskbar();
    }

    private int ScreenW => GetSystemMetrics(0);
    private int ScreenH => GetSystemMetrics(1);

    private async void CreateDesktop()
    {
        _desktop = new Window();
        _desktop.ExtendsContentIntoTitleBar = true;

        var grid = new Grid();
        var wallpaper = new Image { Stretch = Microsoft.UI.Xaml.Media.Stretch.UniformToFill };
        var canvas = new Canvas();
        grid.Children.Add(wallpaper);
        grid.Children.Add(canvas);
        _desktop.Content = grid;

        var hwnd = WindowNative.GetWindowHandle(_desktop);
        NativeMethods.SetWindowLongW(hwnd, NativeMethods.GWL_STYLE,
            unchecked((int)(NativeMethods.WS_POPUP | NativeMethods.WS_VISIBLE)));
        NativeMethods.SetWindowLongW(hwnd, NativeMethods.GWL_EXSTYLE,
            NativeMethods.WS_EX_TOOLWINDOW);
        NativeMethods.SetWindowPos(hwnd, NativeMethods.HWND_BOTTOM,
            0, 0, ScreenW, ScreenH,
            NativeMethods.SWP_FRAMECHANGED | NativeMethods.SWP_NOACTIVATE | NativeMethods.SWP_SHOWWINDOW);

        var sb = new StringBuilder(1024);
        if (NativeMethods.SB_Config_GetWallpaperPath(sb, 1024))
        {
            try { wallpaper.Source = new BitmapImage(new Uri(sb.ToString())); }
            catch { }
        }

        await LoadDesktopIcons(canvas);
    }

    private async Task LoadDesktopIcons(Canvas canvas)
    {
        int count = NativeMethods.SB_Desktop_EnumerateIcons(null!, 0);
        if (count <= 0) return;

        var icons = new SB_DesktopIcon[count];
        int actual = NativeMethods.SB_Desktop_EnumerateIcons(icons, count);
        if (actual <= 0) return;

        for (int i = 0; i < actual; i++)
        {
            var di = icons[i];
            var img = new Image { Width = 48, Height = 48 };
            Canvas.SetLeft(img, di.x + 26);
            Canvas.SetTop(img, di.y + 8);
            canvas.Children.Add(img);

            var label = new TextBlock
            {
                Text = di.name,
                FontSize = 11,
                Foreground = new SolidColorBrush(Colors.White),
                TextAlignment = TextAlignment.Center,
                Width = 100,
                TextWrapping = TextWrapping.Wrap
            };
            Canvas.SetLeft(label, di.x);
            Canvas.SetTop(label, di.y + 60);
            canvas.Children.Add(label);

            var src = await IconHelper.FromPathAsync(di.parsingName);
            if (src != null)
                img.Source = src;
        }
    }

    private void CreateFlyout()
    {
        _flyout = new Window();
        _flyout.ExtendsContentIntoTitleBar = true;
        _flyoutControl = new FlyoutControl();
        _flyout.Content = _flyoutControl;

        var hwnd = WindowNative.GetWindowHandle(_flyout);
        NativeMethods.SetWindowLongW(hwnd, NativeMethods.GWL_STYLE,
            unchecked((int)(NativeMethods.WS_POPUP | NativeMethods.WS_VISIBLE)));
        NativeMethods.SetWindowLongW(hwnd, NativeMethods.GWL_EXSTYLE,
            NativeMethods.WS_EX_TOOLWINDOW | NativeMethods.WS_EX_NOACTIVATE);
        var appWindow = GetAppWindow(_flyout);
        appWindow.Hide();
    }

    private void CreateStartMenu()
    {
        _startMenu = new Window();
        _startMenu.ExtendsContentIntoTitleBar = true;
        _startMenuControl = new StartMenuControl();
        _startMenu.Content = _startMenuControl;

        var hwnd = WindowNative.GetWindowHandle(_startMenu);
        NativeMethods.SetWindowLongW(hwnd, NativeMethods.GWL_STYLE,
            unchecked((int)(NativeMethods.WS_POPUP | NativeMethods.WS_VISIBLE)));
        NativeMethods.SetWindowLongW(hwnd, NativeMethods.GWL_EXSTYLE,
            NativeMethods.WS_EX_TOOLWINDOW | NativeMethods.WS_EX_NOACTIVATE);
        var appWindow = GetAppWindow(_startMenu);
        appWindow.Hide();
    }

    private void CreateTaskbar()
    {
        _taskbar = new Window();
        _taskbar.ExtendsContentIntoTitleBar = true;

        var grid = new Grid();
        var dock = new StackPanel { Orientation = Orientation.Horizontal };

        var startBtn = new Button
        {
            Background = new SolidColorBrush(Colors.Transparent),
            BorderThickness = new Thickness(0),
            Width = 48, Height = 48,
            CornerRadius = new CornerRadius(12)
        };
        startBtn.Content = new FontIcon { Glyph = "\uE71D", FontSize = 18 };
        startBtn.Click += (_, _) => ToggleStartMenu();
        dock.Children.Add(startBtn);

        var rightPanel = new StackPanel
        {
            Orientation = Orientation.Horizontal,
            HorizontalAlignment = HorizontalAlignment.Right,
            VerticalAlignment = VerticalAlignment.Center
        };
        var trayBtn = new Button
        {
            Background = new SolidColorBrush(Colors.Transparent),
            BorderThickness = new Thickness(0),
            Width = 36, Height = 36,
            CornerRadius = new CornerRadius(8)
        };
        trayBtn.Content = new FontIcon { Glyph = "\uE713", FontSize = 18 };
        trayBtn.Click += (_, _) => ToggleFlyout();
        rightPanel.Children.Add(trayBtn);

        _clockBlock = new TextBlock
        {
            Text = DateTime.Now.ToString("h:mm tt"),
            FontSize = 12,
            Foreground = new SolidColorBrush(Color.FromArgb(255, 240, 240, 240)),
            Margin = new Thickness(8, 0, 14, 0),
            VerticalAlignment = VerticalAlignment.Center
        };
        rightPanel.Children.Add(_clockBlock);

        grid.Children.Add(dock);
        grid.Children.Add(rightPanel);
        _taskbar.Content = grid;

        var hwnd = WindowNative.GetWindowHandle(_taskbar);
        NativeMethods.SetWindowLongW(hwnd, NativeMethods.GWL_STYLE,
            unchecked((int)(NativeMethods.WS_POPUP | NativeMethods.WS_VISIBLE)));
        NativeMethods.SetWindowLongW(hwnd, NativeMethods.GWL_EXSTYLE,
            NativeMethods.WS_EX_TOOLWINDOW | NativeMethods.WS_EX_TOPMOST);
        NativeMethods.SetWindowPos(hwnd, NativeMethods.HWND_TOPMOST,
            0, ScreenH - 48, ScreenW, 48,
            NativeMethods.SWP_SHOWWINDOW | NativeMethods.SWP_FRAMECHANGED);

        var abd = new APPBARDATA
        {
            cbSize = Marshal.SizeOf<APPBARDATA>(),
            hWnd = hwnd,
            uEdge = NativeMethods.ABE_BOTTOM,
            rc = new RECT { right = ScreenW, bottom = ScreenH }
        };
        NativeMethods.SHAppBarMessage(NativeMethods.ABM_NEW, ref abd);
        abd.rc.top = ScreenH - 48;
        NativeMethods.SHAppBarMessage(NativeMethods.ABM_SETPOS, ref abd);

        var timer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(1) };
        timer.Tick += (_, _) => { if (_clockBlock != null) _clockBlock.Text = DateTime.Now.ToString("h:mm tt"); };
        timer.Start();
    }

    private void ToggleFlyout()
    {
        if (_flyout == null) return;
        var appWindow = GetAppWindow(_flyout);
        if (appWindow.IsVisible)
        {
            appWindow.Hide();
        }
        else
        {
            var hwnd = WindowNative.GetWindowHandle(_flyout);
            NativeMethods.SetWindowPos(hwnd, NativeMethods.HWND_TOPMOST,
                ScreenW - 370, ScreenH - 48 - 500, 360, 500,
                NativeMethods.SWP_SHOWWINDOW | NativeMethods.SWP_NOACTIVATE);
        }
    }

    private void ToggleStartMenu()
    {
        if (_startMenu == null) return;
        var appWindow = GetAppWindow(_startMenu);
        if (appWindow.IsVisible)
        {
            appWindow.Hide();
        }
        else
        {
            var hwnd = WindowNative.GetWindowHandle(_startMenu);
            NativeMethods.SetWindowPos(hwnd, NativeMethods.HWND_TOPMOST,
                10, ScreenH - 48 - 620, 640, 620,
                NativeMethods.SWP_SHOWWINDOW | NativeMethods.SWP_NOACTIVATE);
        }
    }

    private static AppWindow GetAppWindow(Window window)
    {
        var hwnd = WindowNative.GetWindowHandle(window);
        var windowId = Win32Interop.GetWindowIdFromWindow(hwnd);
        return AppWindow.GetFromWindowId(windowId);
    }

    [DllImport("user32.dll")]
    private static extern int GetSystemMetrics(int nIndex);
}
