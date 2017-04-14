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
            public override void OnPluginAdded(UInt32 idx)
            {

            }

            public override void OnPluginDeleted(UInt32 idx)
            {

            }

            public override void OnListLoaded()
            {

            }

            public override void OnMovedUp(UInt32 idx)
            {

            }

            public override void OnMovedDown(UInt32 idx)
            {

            }

            public override void OnEditorShown(UInt32 idx)
            {

            }

            public override void OnEditorHidden(UInt32 idx)
            {

            }

            public override void OnPresetSet(UInt32 plugin_idx, UInt32 preset_idx)
            {

            }

            public override void OnBypassSet(UInt32 idx, bool bypass)
            {

            }
        }
        private HostControllerProxy hcp;
        public VSTHostWindow(HostControllerProxy hcp_)
        {
            InitializeComponent();
            hcp = hcp_;
        }

        private void buttonShow_Click(object sender, RoutedEventArgs e)
        {
            VSTPluginWindow pw = new VSTPluginWindow();
            pw.Show();
        }
    }
}
