using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Windows.Graphics.Imaging;
using Windows.Storage.Streams;

namespace WinUIShell;

internal static class IconHelper
{
    public static async Task<ImageSource?> FromHiconAsync(IntPtr hIcon)
    {
        if (hIcon == IntPtr.Zero) return null;
        try
        {
            using var icon = Icon.FromHandle(hIcon);
            using var bitmap = icon.ToBitmap();
            return await BitmapToImageSourceAsync(bitmap);
        }
        catch
        {
            return null;
        }
    }

    public static async Task<ImageSource?> FromPathAsync(string path)
    {
        if (string.IsNullOrEmpty(path)) return null;
        try
        {
            var fi = new SHFILEINFOW();
            // Try without USEFILEATTRIBUTES first — this resolves .lnk targets
            // and returns the actual app's icon instead of the generic shortcut icon.
            IntPtr ret = NativeMethods.SHGetFileInfoW(path, 0, out fi,
                Marshal.SizeOf<SHFILEINFOW>(),
                NativeMethods.SHGFI_ICON | NativeMethods.SHGFI_LARGEICON);
            if (ret == IntPtr.Zero || fi.hIcon == IntPtr.Zero)
            {
                // Fallback: use USEFILEATTRIBUTES for non-existent / virtual paths
                ret = NativeMethods.SHGetFileInfoW(path, 0x80 /*FILE_ATTRIBUTE_NORMAL*/, out fi,
                    Marshal.SizeOf<SHFILEINFOW>(),
                    NativeMethods.SHGFI_ICON | NativeMethods.SHGFI_LARGEICON | NativeMethods.SHGFI_USEFILEATTRIBUTES);
            }
            if (ret == IntPtr.Zero || fi.hIcon == IntPtr.Zero) return null;

            using var icon = Icon.FromHandle(fi.hIcon);
            using var bitmap = icon.ToBitmap();
            NativeMethods.DestroyIcon(fi.hIcon);
            return await BitmapToImageSourceAsync(bitmap);
        }
        catch
        {
            return null;
        }
    }

    private static async Task<ImageSource?> BitmapToImageSourceAsync(Bitmap bitmap)
    {
        var rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        var data = bitmap.LockBits(rect, ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);

        var buffer = new byte[Math.Abs(data.Stride) * bitmap.Height];
        Marshal.Copy(data.Scan0, buffer, 0, buffer.Length);
        bitmap.UnlockBits(data);

        using var stream = new InMemoryRandomAccessStream();
        var encoder = await BitmapEncoder.CreateAsync(BitmapEncoder.PngEncoderId, stream);
        encoder.SetPixelData(
            BitmapPixelFormat.Bgra8,
            BitmapAlphaMode.Premultiplied,
            (uint)bitmap.Width,
            (uint)bitmap.Height,
            96, 96,
            buffer);
        await encoder.FlushAsync();

        stream.Seek(0);
        var decoder = await BitmapDecoder.CreateAsync(stream);
        var decoded = await decoder.GetSoftwareBitmapAsync();
        var source = new SoftwareBitmapSource();
        await source.SetBitmapAsync(decoded);
        return source;
    }
}
