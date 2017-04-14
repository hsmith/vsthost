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
using System.Windows.Navigation;
using System.Windows.Shapes;

using VSTHost;

namespace vsthost_tester_csharp_wpf
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    /// 
    public partial class MainWindow : Window
    {
        private vsthost_tester_csharp.Tester tester = new vsthost_tester_csharp.Tester();
        private VSTHostWindow hw;
        public MainWindow()
        {
            InitializeComponent();
            tester.Initialize();
            hw = new VSTHostWindow(tester.GetController());
        }

        private void buttonPlay_Click(object sender, RoutedEventArgs e)
        {
            tester.Play();
        }
        private void buttonProcess_Click(object sender, RoutedEventArgs e)
        {
            tester.Process();
        }
        private void buttonStop_Click(object sender, RoutedEventArgs e)
        {
            tester.Stop();
        }

        private void buttonShow_Click(object sender, RoutedEventArgs e)
        {
            hw.Show();
        }
    }
}
