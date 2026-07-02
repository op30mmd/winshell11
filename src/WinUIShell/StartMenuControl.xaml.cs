using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using System.Collections.ObjectModel;
using System.Threading.Tasks;

namespace WinUIShell;

public sealed partial class StartMenuControl : UserControl
{
    private readonly ObservableCollection<AppItemModel> _filteredApps = new();
    private System.Collections.Generic.List<AppItemModel>? _allApps;

    public StartMenuControl()
    {
        InitializeComponent();
        LoadApps();
        AppsListView.ItemsSource = _filteredApps;
    }

    private async void LoadApps()
    {
        int count = NativeMethods.SB_Launcher_Enumerate(null!, 0);
        if (count <= 0) return;

        var raw = new SB_AppItem[count];
        int actual = NativeMethods.SB_Launcher_Enumerate(raw, count);
        if (actual <= 0) return;

        _allApps = new System.Collections.Generic.List<AppItemModel>(actual);
        var batch = new System.Collections.Generic.List<(AppItemModel model, string iconPath)>(actual);

        for (int i = 0; i < actual; i++)
        {
            var model = new AppItemModel
            {
                Name = raw[i].name,
                Path = raw[i].path,
                Target = raw[i].target,
                Type = (AppType)raw[i].type
            };
            // Use resolved target for icon extraction, fall back to .lnk path
            string iconPath = !string.IsNullOrEmpty(raw[i].target) ? raw[i].target : raw[i].path;
            batch.Add((model, iconPath));
            _allApps.Add(model);
        }

        _allApps.Sort((a, b) => string.Compare(a.Name, b.Name, System.StringComparison.OrdinalIgnoreCase));
        batch.Sort((a, b) => string.Compare(a.model.Name, b.model.Name, System.StringComparison.OrdinalIgnoreCase));

        foreach (var (model, iconPath) in batch)
        {
            model.Icon = await IconHelper.FromPathAsync(iconPath);
            _filteredApps.Add(model);
        }
    }

    private void OnSearchChanged(object sender, TextChangedEventArgs e)
    {
        string filter = SearchBox.Text.Trim().ToLower();
        _filteredApps.Clear();
        if (_allApps == null) return;
        foreach (var app in _allApps)
        {
            if (string.IsNullOrEmpty(filter) || app.Name.ToLower().Contains(filter))
                _filteredApps.Add(app);
        }
    }

    private void OnAppClick(object sender, RoutedEventArgs e)
    {
        if (sender is Button { Tag: string path })
            NativeMethods.SB_Launcher_Launch(path);
    }
}
