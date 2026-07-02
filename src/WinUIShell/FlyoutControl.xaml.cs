using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using System;
using System.Diagnostics;
using System.Text;
using Windows.UI;

namespace WinUIShell;

public sealed partial class FlyoutControl : UserControl
{
    private bool _loading;

    private static readonly SolidColorBrush ActiveToggleBg = new(Color.FromArgb(0x33, 0x60, 0xCD, 0xFF));
    private static readonly SolidColorBrush InactiveToggleBg = new(Colors.Transparent);

    public FlyoutControl()
    {
        InitializeComponent();
        LoadState();
    }

    public void LoadState()
    {
        _loading = true;

        VolumeSlider.Value = NativeMethods.SB_Volume_Get();
        VolumeLabel.Text = $"Volume  {(int)VolumeSlider.Value}%";

        bool hasBrightness = NativeMethods.SB_Brightness_HasControl();
        if (hasBrightness)
        {
            BrightnessSlider.Value = NativeMethods.SB_Brightness_Get();
            BrightnessLabel.Text = $"Brightness  {(int)BrightnessSlider.Value}%";
            BrightnessPanel.Visibility = Visibility.Visible;
        }
        else
        {
            BrightnessPanel.Visibility = Visibility.Collapsed;
        }

        // Hide BT button if no hardware detected
        bool hasBt = NativeMethods.SB_Bluetooth_HasHardware();
        BtButton.Visibility = hasBt ? Visibility.Visible : Visibility.Collapsed;

        UpdateWifiState();
        UpdateBtState();

        var sb = new StringBuilder(256);
        if (NativeMethods.SB_Network_GetInfo(sb, 256))
            NetworkInfo.Text = sb.ToString();

        _loading = false;
    }

    private void UpdateWifiState()
    {
        try
        {
            bool enabled = NativeMethods.SB_WiFi_IsEnabled();
            WifiButton.Background = enabled ? ActiveToggleBg : InactiveToggleBg;
            WifiLabel.Text = enabled ? "Wi-Fi  On" : "Wi-Fi  Off";
        }
        catch (Exception ex)
        {
            Debug.WriteLine($"WiFi state error: {ex.Message}");
        }
    }

    private void UpdateBtState()
    {
        try
        {
            bool enabled = NativeMethods.SB_Bluetooth_IsEnabled();
            BtButton.Background = enabled ? ActiveToggleBg : InactiveToggleBg;
            BtLabel.Text = enabled ? "Bluetooth  On" : "Bluetooth  Off";
        }
        catch (Exception ex)
        {
            Debug.WriteLine($"BT state error: {ex.Message}");
        }
    }

    private void OnVolumeChanged(object sender, RangeBaseValueChangedEventArgs e)
    {
        if (_loading) return;
        int val = (int)e.NewValue;
        VolumeLabel.Text = $"Volume  {val}%";
        NativeMethods.SB_Volume_Set(val);
    }

    private void OnBrightnessChanged(object sender, RangeBaseValueChangedEventArgs e)
    {
        if (_loading) return;
        int val = (int)e.NewValue;
        BrightnessLabel.Text = $"Brightness  {val}%";
        NativeMethods.SB_Brightness_Set(val);
    }

    private void OnWifiToggle(object sender, RoutedEventArgs e)
    {
        NativeMethods.SB_WiFi_Toggle();
        UpdateWifiState();
    }

    private void OnBtToggle(object sender, RoutedEventArgs e)
    {
        NativeMethods.SB_Bluetooth_Toggle();
        UpdateBtState();
    }

    private void OnLock(object sender, RoutedEventArgs e) => NativeMethods.SB_Power_Lock();
    private void OnSleep(object sender, RoutedEventArgs e) => NativeMethods.SB_Power_Sleep();
    private void OnRestart(object sender, RoutedEventArgs e) => NativeMethods.SB_Power_Restart();
    private void OnShutdown(object sender, RoutedEventArgs e) => NativeMethods.SB_Power_Shutdown();
}
