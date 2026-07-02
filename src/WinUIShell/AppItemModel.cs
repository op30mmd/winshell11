using Microsoft.UI.Xaml.Media;

namespace WinUIShell;

public enum AppType
{
    Application = 0,
    ControlPanel = 1,
    SystemTool = 2,
    WebLink = 3,
    Unknown = 4
}

public class AppItemModel
{
    public string Name { get; set; } = "";
    public string Path { get; set; } = "";
    public string Target { get; set; } = "";
    public AppType Type { get; set; } = AppType.Unknown;
    public ImageSource? Icon { get; set; }
    public string CategoryLabel => Type switch
    {
        AppType.Application => "Application",
        AppType.ControlPanel => "System",
        AppType.SystemTool => "System",
        AppType.WebLink => "Web Link",
        _ => "Other"
    };
}
