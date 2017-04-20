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
    /// Interaction logic for VSTPluginWindow.xaml
    /// </summary>
    public partial class VSTPluginWindow : Window
    {
        private HostControllerProxy hcp;
        private uint index;

        private System.Windows.Forms.Integration.WindowsFormsHost host = new System.Windows.Forms.Integration.WindowsFormsHost();
        private System.Windows.Forms.UserControl uc = new System.Windows.Forms.UserControl();

        private bool size_fixed = false;

        public VSTPluginWindow(HostControllerProxy hcp_, uint i)
        {
            InitializeComponent();
            hcp = hcp_;
            index = i;

            SizeToContent = SizeToContent.WidthAndHeight;
            Title = hcp.GetPluginName(index);
            menuItemBypass.IsChecked = hcp.IsBypassed(index);
            menuItemActive.IsChecked = hcp.IsActive(index);

            hcp.CreateEditor(index, uc.Handle);
            FixSize();
            GeneratePresetMenu();
        }

        public void ShowEditor()
        {
            hcp.ShowEditor(index);
            if (size_fixed == false)
            {
                FixSize();
                size_fixed = true;
            }
            Show();
        }

        public void HideEditor()
        {
            hcp.HideEditor(index);
            Hide();
        }

        public void MovedUp()
        {
            index--;
        }

        public void MovedDown()
        {
            index++;
        }

        public void BypassSet(bool bypass)
        {
            menuItemBypass.IsChecked = bypass;
        }

        public void ActiveSet(bool active)
        {
            menuItemActive.IsChecked = active;
        }

        public void PresetSet(uint idx)
        {
            var idx_ = Convert.ToInt32(idx);
            for (int i = 0; i < menuItemPresets.Items.Count; ++i)
                (menuItemPresets.Items[i] as MenuItem).IsChecked = i == idx_;
        }

        private void GeneratePresetMenu()
        {
            if (hcp.GetPluginPresetCount(index) > 0)
            {
                for (uint i = 0; i < hcp.GetPluginPresetCount(index); ++i)
                {
                    MenuItem item = new MenuItem();
                    item.Header = hcp.GetPluginPresetName(index, i);
                    item.Click += new RoutedEventHandler(menuItemPreset_Click);
                    menuItemPresets.Items.Add(item);
                }
            }
            else
                menuItemPresets.IsEnabled = false;
        }

        private void FixSize()
        {
            var w = Convert.ToInt32(hcp.GetPluginEditorWidth(index));
            var h = Convert.ToInt32(hcp.GetPluginEditorHeight(index));
            if (w != uc.Width)
            {
                uc.Width = w;
            }
            if (w != grid.Width)
            {
                grid.Width = w;
            }
            if (h != uc.Height)
            {
                uc.Height = h;
            }
            if (h != grid.Height)
            {
                grid.Height = h;
            }
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            e.Cancel = true;
            HideEditor();
        }

        private void menuItemBypass_Click(object sender, RoutedEventArgs e)
        {
            hcp.SetBypass(index, !menuItemBypass.IsChecked);
        }

        private void menuItemActive_Click(object sender, RoutedEventArgs e)
        {
            hcp.SetActive(index, !menuItemActive.IsChecked);
        }

        private void menuItemClose_Click(object sender, RoutedEventArgs e)
        {
            HideEditor();
        }

        private void menuItemLoad_Click(object sender, RoutedEventArgs e)
        {
            hcp.LoadPreset(index);
        }

        private void menuItemSave_Click(object sender, RoutedEventArgs e)
        {
            hcp.SavePreset(index);
        }

        private void menuItemPreset_Click(object sender, RoutedEventArgs e)
        {
            for (int i = 0; i < menuItemPresets.Items.Count; ++i)
            {
                if (sender == menuItemPresets.Items[i])
                {
                    hcp.SetPluginPreset(index, Convert.ToUInt32(i));
                    break;
                }
            }
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            host.Child = uc;
            grid.Children.Add(host);
        }
    }
}
