using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

using VSTHost;

namespace vsthost_tester_csharp_wpf
{
    /// <summary>
    /// Interaction logic for VSTHostWindow.xaml
    /// </summary>
    /// 
    

    public partial class VSTHostWindow : Window
    {
        private class HostObserverImpl : HostObserverProxyWrapper
        {
            private VSTHostWindow parent;

            public HostObserverImpl(VSTHostWindow p)
            {
                parent = p;
            }
            public override void OnPluginAdded(UInt32 idx)
            {
                parent.OnPluginAdded(idx);
            }

            public override void OnPluginDeleted(UInt32 idx)
            {
                parent.OnPluginDeleted(idx);
            }

            public override void OnListLoaded()
            {
                parent.OnListLoaded();
            }

            public override void OnMovedUp(UInt32 idx)
            {
                parent.OnMovedUp(idx);
            }

            public override void OnMovedDown(UInt32 idx)
            {
                parent.OnMovedDown(idx);
            }

            public override void OnEditorShown(UInt32 idx)
            {
                parent.OnEditorShown(idx);
            }

            public override void OnEditorHidden(UInt32 idx)
            {
                parent.OnEditorHidden(idx);
            }

            public override void OnPresetSet(UInt32 plugin_idx, UInt32 preset_idx)
            {
                parent.OnPresetSet(plugin_idx, preset_idx);
            }

            public override void OnBypassSet(UInt32 idx, bool bypass)
            {
                parent.OnBypassSet(idx, bypass);
            }

            public override void OnActiveSet(UInt32 idx, bool active)
            {
                parent.OnActiveSet(idx, active);
            }
        }
        private Microsoft.Win32.OpenFileDialog ofd = new Microsoft.Win32.OpenFileDialog();
        private HostObserverImpl hoi;
        private HostControllerProxy hcp;
        private List<VSTPluginWindow> pluginWindows;

        public VSTHostWindow(HostControllerProxy hcp_)
        {
            InitializeComponent();
            hoi = new HostObserverImpl(this);
            hcp = hcp_;
            hcp.RegisterObserver(hoi);
            
            ofd.Filter = "VST Plugins (*.dll, *.vst3)|*.dll;*.vst3|VST2 Plugins (*.dll)|*.dll|VST3 Plugins (*.vst3)|*.vst3";
            ofd.InitialDirectory = System.IO.Path.GetFullPath(HostProxy.PluginDirectory);

            Initialize();
            SetButtons();
        }

        ~VSTHostWindow()
        {
            hcp.UnregisterObserver(hoi);
        }

        private int Selected()
        {
            return listPlugins.SelectedIndex;
        }

        private void Select(uint i)
        {
            listPlugins.SelectedIndex = Convert.ToInt32(i);
            listPlugins.Focus();
        }

        private void Initialize()
        {
            pluginWindows = new List<VSTPluginWindow>();
            listPlugins.Items.Clear();
            for (uint i = 0; i < hcp.GetPluginCount(); ++i)
            {
                pluginWindows.Add(new VSTPluginWindow(hcp, i));
                listPlugins.Items.Add(hcp.GetPluginName(i));
            }
            if (hcp.GetPluginCount() > 0)
            {
                Select(0);
                SetButtons();
            }
        }

        private void SetButtons()
        {
            var count = hcp.GetPluginCount();
            var selected = Selected();

            buttonUp.IsEnabled = count >= 2 && selected > 0;
            buttonDown.IsEnabled = count >= 2 && selected >= 0 && selected < count - 1;
            buttonDelete.IsEnabled = count > 0;

            if (count > 0 && selected >= 0 && hcp.HasEditor(Convert.ToUInt32(selected)))
            {
                buttonShow.IsEnabled = true;
                if (pluginWindows[selected].IsVisible == true)
                    buttonShow.Content = "Hide";
                else
                    buttonShow.Content = "Show";
            }
            else
            {
                buttonShow.IsEnabled = false;
                buttonShow.Content = "Show";
            }
        }

        private void buttonShow_Click(object sender, RoutedEventArgs e)
        {
            var selected = Selected();
            if (pluginWindows[selected].IsVisible == true)
                pluginWindows[selected].HideEditor();
            else
                pluginWindows[selected].ShowEditor();
        }

        private void OnPluginAdded(UInt32 idx)
        {
            pluginWindows.Add(new VSTPluginWindow(hcp, idx));
            listPlugins.Items.Add(hcp.GetPluginName(idx));
            Select(idx);
            SetButtons();
        }

        private void OnPluginDeleted(UInt32 idx)
        {
            var i = Convert.ToInt32(idx);
            if (pluginWindows[i].IsVisible == true)
                pluginWindows[i].HideEditor();
            pluginWindows.RemoveAt(i);
            listPlugins.Items.RemoveAt(i);

            var count = hcp.GetPluginCount();
            if (count > 0)
            {
                if (idx == count)
                    Select(idx - 1);
                else
                    Select(idx);
            }
            else
                listPlugins.SelectedIndex = -1;
            SetButtons();
        }

        private void OnListLoaded()
        {
            Initialize();
        }

        private void OnMovedUp(UInt32 idx)
        {
            var i = Convert.ToInt32(idx);
            pluginWindows[i].MovedUp();
            pluginWindows[i - 1].MovedDown();

            var tmp = pluginWindows[i];
            pluginWindows[i] = pluginWindows[i - 1];
            pluginWindows[i - 1] = tmp;

            var tmp1 = listPlugins.Items[i];
            listPlugins.Items[i] = listPlugins.Items[i - 1];
            listPlugins.Items[i - 1] = tmp1;

            Select(idx - 1);
            SetButtons();
        }

        private void OnMovedDown(UInt32 idx)
        {
            var i = Convert.ToInt32(idx);
            pluginWindows[i].MovedDown();
            pluginWindows[i + 1].MovedUp();

            var tmp = pluginWindows[i];
            pluginWindows[i] = pluginWindows[i + 1];
            pluginWindows[i + 1] = tmp;

            var tmp1 = listPlugins.Items[i];
            listPlugins.Items[i] = listPlugins.Items[i + 1];
            listPlugins.Items[i + 1] = tmp1;

            Select(idx + 1);
            SetButtons();
        }

        private void OnEditorShown(UInt32 idx)
        {
            if (idx == Selected())
                buttonShow.Content = "Hide";
        }

        private void OnEditorHidden(UInt32 idx)
        {
            if (idx == Selected())
                buttonShow.Content = "Show";
        }

        private void OnPresetSet(UInt32 plugin_idx, UInt32 preset_idx)
        {
            pluginWindows[Convert.ToInt32(plugin_idx)].PresetSet(preset_idx);
        }

        private void OnBypassSet(UInt32 idx, bool bypass)
        {
            pluginWindows[Convert.ToInt32(idx)].BypassSet(bypass);
        }

        private void OnActiveSet(UInt32 idx, bool active)
        {
            pluginWindows[Convert.ToInt32(idx)].ActiveSet(active);
        }

        private void listPlugins_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            SetButtons();
        }

        private void buttonAdd_Click(object sender, RoutedEventArgs e)
        {
            var result = ofd.ShowDialog();
            if (result == true)
            {
                hcp.AddPlugin(ofd.FileName);
            }
        }

        private void buttonDelete_Click(object sender, RoutedEventArgs e)
        {
            hcp.DeletePlugin(Convert.ToUInt32(Selected()));
        }

        private void buttonUp_Click(object sender, RoutedEventArgs e)
        {
            hcp.MoveUp(Convert.ToUInt32(Selected()));
        }

        private void buttonDown_Click(object sender, RoutedEventArgs e)
        {
            hcp.MoveDown(Convert.ToUInt32(Selected()));
        }

        private void buttonLoad_Click(object sender, RoutedEventArgs e)
        {
            hcp.LoadPluginList();
        }

        private void buttonSave_Click(object sender, RoutedEventArgs e)
        {
            hcp.SavePluginList();
        }
    }
}
