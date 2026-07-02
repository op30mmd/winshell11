using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Data;

namespace WinUIShell;

internal class ShowCategoryConverter : IValueConverter
{
    public object Convert(object value, System.Type targetType, object parameter, string language)
    {
        if (value is string category && category == "Application")
            return Visibility.Collapsed;
        return Visibility.Visible;
    }

    public object ConvertBack(object value, System.Type targetType, object parameter, string language)
    {
        throw new System.NotImplementedException();
    }
}