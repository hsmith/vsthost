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

namespace vsthost_tester_csharp_wpf
{
    /// <summary>
    /// Interaction logic for VSTHostWindow.xaml
    /// </summary>
    public partial class VSTHostWindow : Window
    {
        public VSTHostWindow()
        {
            InitializeComponent();
        }

        private void buttonShow_Click(object sender, RoutedEventArgs e)
        {
            VSTPluginWindow pw = new VSTPluginWindow();
            pw.Show();
        }
    }
}
